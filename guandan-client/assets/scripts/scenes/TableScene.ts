import { _decorator, Component, Button, Label, director } from 'cc';
import { session } from '../game/Session';

const { ccclass, property } = _decorator;

// 牌桌（房间）场景：目前为占位——游戏流程走 ConsoleView（控制台文字）。
// 后续实现 TableView 接口的 Cocos 组件，替换 Session 里的 ConsoleView 即可接上真实牌桌 UI。
@ccclass('TableScene')
export class TableScene extends Component {
  @property(Button) backBtn: Button = null!;
  @property(Label) statusLabel: Label = null!;

  onLoad(): void {
    this.backBtn.node.on(Button.EventType.CLICK, this.onBackClicked, this);
    if (this.statusLabel) {
      this.statusLabel.string = '房间号：' + session.roomId;
    }
  }

  private onBackClicked(): void {
    director.loadScene('Lobby');
  }
}
