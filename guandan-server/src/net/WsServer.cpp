#include "net/WsServer.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <vector>

#include "util/Log.h"
#include "util/Time.h"

namespace guandan::net {

namespace {
bool setNonBlocking(int fd) {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags < 0) return false;
  return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
}  // namespace

WsServer::WsServer() = default;
WsServer::~WsServer() { stop(); }

bool WsServer::listen(uint16_t port) {
  listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd_ < 0) return false;

  int one = 1;
  ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#ifdef SO_NOSIGPIPE
  ::setsockopt(listenFd_, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) return false;
  if (::listen(listenFd_, 64) < 0) return false;
  if (!setNonBlocking(listenFd_)) return false;

  sockaddr_in bound{};
  socklen_t blen = sizeof(bound);
  if (::getsockname(listenFd_, reinterpret_cast<sockaddr*>(&bound), &blen) == 0) {
    port_ = ntohs(bound.sin_port);
  }

  if (::pipe(wakeup_) < 0) return false;
  setNonBlocking(wakeup_[0]);
  setNonBlocking(wakeup_[1]);
  return true;
}

void WsServer::acceptPending() {
  while (true) {
    sockaddr_in peer{};
    socklen_t plen = sizeof(peer);
    const int fd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&peer), &plen);
    if (fd < 0) {
      if (errno == EINTR) continue;
      return;  // EAGAIN 或错误
    }
    setNonBlocking(fd);
    int one = 1;
#ifdef SO_NOSIGPIPE
    ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    auto session = std::make_unique<WsSession>(fd);
    session->setLastHeartbeatMs(util::nowMs());
    WsSession* raw = session.get();
    sessions_[fd] = std::move(session);
    if (onConnect_) onConnect_(*raw);
  }
}

void WsServer::prune() {
  std::vector<int> toRemove;
  for (auto& [fd, s] : sessions_) {
    if (s->state() == SessionState::Closing && !s->wantWrite()) toRemove.push_back(fd);
  }
  for (const int fd : toRemove) {
    auto it = sessions_.find(fd);
    if (it != sessions_.end()) {
      if (onDisconnect_) onDisconnect_(*it->second);
      sessions_.erase(it);
    }
  }
}

void WsServer::sweepHeartbeats(int64_t nowMs) {
  for (auto& [fd, s] : sessions_) {
    if (s->state() != SessionState::Open) continue;
    if (nowMs - s->lastHeartbeatMs() > kHeartbeatIntervalMs * kMaxMissedHeartbeats) {
      GD_LOG_WARN("heartbeat timeout, close fd={} playerId={}", fd, s->playerId());
      s->close();
    }
  }
}

void WsServer::run() {
  running_ = true;
  while (running_) {
    std::vector<pollfd> fds;
    fds.reserve(sessions_.size() + 2);
    pollfd lp{};
    lp.fd = listenFd_;
    lp.events = POLLIN;
    fds.push_back(lp);
    pollfd wp{};
    wp.fd = wakeup_[0];
    wp.events = POLLIN;
    fds.push_back(wp);
    for (auto& [fd, s] : sessions_) {
      pollfd p{};
      p.fd = fd;
      p.events = POLLIN;
      if (s->wantWrite()) p.events |= POLLOUT;
      fds.push_back(p);
    }

    const int ret = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), 200);
    if (ret < 0) {
      if (errno == EINTR) continue;
      break;
    }

    if (fds[1].revents & POLLIN) {
      uint8_t buf[16];
      while (::read(wakeup_[0], buf, sizeof(buf)) > 0) {}
      if (!running_) break;
    }
    if (fds[0].revents & POLLIN) acceptPending();

    for (size_t i = 2; i < fds.size(); ++i) {
      const int fd = fds[i].fd;
      auto it = sessions_.find(fd);
      if (it == sessions_.end()) continue;
      auto& s = *it->second;
      const short rev = fds[i].revents;
      if (rev & (POLLERR | POLLHUP | POLLNVAL)) s.close();
      if (rev & POLLIN) {
        uint8_t buf[4096];
        while (true) {
          const ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
          if (n > 0) {
            s.appendInput(buf, static_cast<size_t>(n));
            if (!s.process(onBinary_)) {
              s.close();
              break;
            }
          } else if (n == 0) {
            s.close();
            break;
          } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            s.close();
            break;
          }
        }
      }
      if (rev & POLLOUT) s.flush();
      if (s.wantWrite()) s.flush();
    }

    sweepHeartbeats(util::nowMs());
    prune();
  }
}

void WsServer::stop() {
  running_ = false;
  if (wakeup_[1] >= 0) {
    const uint8_t b = 1;
    ::write(wakeup_[1], &b, 1);
  }
}

}  // namespace guandan::net
