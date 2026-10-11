import { _decorator, Component, Button, EditBox, Label, director } from 'cc';
import { session } from '../game/Session';

const { ccclass, property } = _decorator;

// 大厅场景：创建房间 / 加入房间 / 匹配 / 退出登录。
@ccclass('LobbyScene')
export class LobbyScene extends Component {
  @property(Button) createBtn: Button = null!;
  @property(Button) joinBtn: Button = null!;
  @property(Button) matchBtn: Button = null!;
  @property(Button) logoutBtn: Button = null!;
  @property(EditBox) roomIdBox: EditBox = null!;
  @property(EditBox) levelBox: EditBox = null!;
  @property(Label) statusLabel: Label = null!;

  onLoad(): void {
    this.createBtn.node.on(Button.EventType.CLICK, this.onCreateClicked, this);
    this.joinBtn.node.on(Button.EventType.CLICK, this.onJoinClicked, this);
    this.matchBtn.node.on(Button.EventType.CLICK, this.onMatchClicked, this);
    this.logoutBtn.node.on(Button.EventType.CLICK, this.onLogoutClicked, this);
    session.on('roomEntered', this.onRoomEntered, this);
    session.on('logout', this.onLogoutDone, this);
    session.on('error', this.onError, this);
  }

  onDestroy(): void {
    session.off('roomEntered', this.onRoomEntered, this);
    session.off('logout', this.onLogoutDone, this);
    session.off('error', this.onError, this);
  }

  private onCreateClicked(): void {
    const level = parseInt(this.levelBox.string, 10) || 2;
    this.statusLabel.string = '创建房间中...';
    session.createRoom(level);
  }

  private onJoinClicked(): void {
    const roomId = this.roomIdBox.string.trim();
    if (!roomId) {
      this.statusLabel.string = '请输入房间号';
      return;
    }
    this.statusLabel.string = '加入房间中...';
    session.joinRoom(roomId);
  }

  private onMatchClicked(): void {
    this.statusLabel.string = '匹配中...（30s 无真人将补 Bot）';
    session.match();
  }

  private onLogoutClicked(): void {
    session.logout();
  }

  private onRoomEntered(): void {
    director.loadScene('Table');
  }

  private onLogoutDone(): void {
    director.loadScene('Login');
  }

  private onError(msg?: unknown): void {
    this.statusLabel.string = '错误：' + String(msg ?? '');
  }
}
