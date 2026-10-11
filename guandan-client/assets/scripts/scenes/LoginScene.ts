import { _decorator, Component, Button, EditBox, director } from 'cc';
import { session } from '../game/Session';

const { ccclass, property } = _decorator;

// 登录场景：输入 uid/token 点登录；登录成功自动切到 Lobby。
@ccclass('LoginScene')
export class LoginScene extends Component {
  @property(EditBox) uidBox: EditBox = null!;
  @property(EditBox) tokenBox: EditBox = null!;
  @property(Button) loginBtn: Button = null!;

  onLoad(): void {
    this.loginBtn.node.on(Button.EventType.CLICK, this.onLoginClicked, this);
    session.on('login', this.onLoginOk, this);
  }

  onDestroy(): void {
    session.off('login', this.onLoginOk, this);
  }

  private onLoginClicked(): void {
    const uid = this.uidBox.string.trim() || 'guest';
    const token = this.tokenBox.string.trim() || 'token';
    session.login(uid, token);
  }

  private onLoginOk(): void {
    director.loadScene('Lobby');
  }
}
