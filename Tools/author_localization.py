"""Author UTF-8 language packs. English source text is the stable key; {n} are arguments."""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
# Columns: English source key, Simplified Chinese, Japanese, Traditional Chinese.
ENTRIES = [
    ("Mouse Sensitivity", "鼠标灵敏度", "マウス感度", "滑鼠靈敏度"),
    ("Lower is slower. Changes are saved automatically.", "数值越低，视角移动越慢。设置会自动保存。", "値が小さいほど視点移動が遅くなります。変更は自動保存されます。", "數值越低，視角移動越慢。設定會自動儲存。"),
    ("Mouse sensitivity: %sx", "鼠标灵敏度：{0} 倍", "マウス感度：{0} 倍", "滑鼠靈敏度：{0} 倍"),
    ("Restore Default", "恢复默认", "初期値に戻す", "恢復預設"),
    ("Back", "返回", "戻る", "返回"),
    ("[Left / Right] Adjust    [Up / Down] Select    [Esc] Back", "[← / →] 调整    [↑ / ↓] 选择    [Esc] 返回", "[← / →] 調整    [↑ / ↓] 選択    [Esc] 戻る", "[← / →] 調整    [↑ / ↓] 選擇    [Esc] 返回"),
    ("CURRENT OBJECTIVE", "当前目标", "現在の目標", "目前目標"),
    ("Time left: %s", "剩余时间：{0}", "残り時間：{0}", "剩餘時間：{0}"),
    ("%s · %s", "{0} · {1}", "{0} · {1}", "{0} · {1}"),
    ("%d overdue chores are draining sanity!", "{0} 项任务超时，正在扣除清醒度！", "期限切れの作業 {0} 件で正気度が減少中！", "{0} 項工作逾時，正在扣除清醒度！"),
    ("[TAB] Next chore", "[Tab] 切换任务", "[Tab] 次の作業", "[Tab] 切換工作"),
    ("Route blocked. Return to an aisle.", "路线受阻，请返回通道。", "経路が塞がれています。通路に戻ってください。", "路線受阻，請返回通道。"),
    ("Waiting for the next customer.", "等待下一位顾客进店。", "次のお客様を待っています。", "等待下一位顧客進店。"),
    ("Customer is leaving. Keep the doorway clear.", "顾客正在离店，请保持出口畅通。", "お客様が退店します。出口を空けてください。", "顧客正在離店，請保持出口暢通。"),
    ("Take the product from a shelf, then press [E] on the customer.", "从货架拿取商品，再对顾客按 [E] 递交。", "棚から商品を取り、お客様に [E] で渡してください。", "從貨架拿取商品，再對顧客按 [E] 遞交。"),
    ("Excuse me, could I get through?", "不好意思，借过一下。", "すみません、通してもらえますか。", "不好意思，借過一下。"),
    ("Let's get the store ready.", "先把店里的活干完。", "まずは店の仕事を片づけよう。", "先把店裡的工作做完。"),
    ("What... was that?", "刚才……那是什么？", "今の……何だった？", "剛才……那是什麼？"),
    ("Let go of me!", "放开我！", "離せ！", "放開我！"),
    ("I need some distance...", "得先拉开距离……", "距離を取らないと……", "得先拉開距離……"),
    ("Stay calm. Don't panic.", "冷静，别慌。", "落ち着け。慌てるな。", "冷靜，別慌。"),
    ("PAUSED", "游戏已暂停", "一時停止", "遊戲已暫停"),
    ("Resume Game", "继续游戏", "ゲームを再開", "繼續遊戲"),
    ("Return to Main Menu", "返回主菜单", "タイトルへ戻る", "返回主選單"),
    ("Quit Game", "退出游戏", "ゲームを終了", "退出遊戲"),
    ("Only dawn checkpoints are saved. Returning to the menu discards this night's progress.", "进度仅在天亮时保存，返回主菜单将丢失本晚未保存的进度。", "夜明けにのみセーブされます。タイトルに戻ると今夜の未保存の進行は失われます。", "進度僅在天亮時儲存，返回主選單將遺失本晚未儲存的進度。"),
    ("[Esc] Resume    [Up / Down] Select    [Enter / Click] Confirm", "[Esc] 继续    [↑ / ↓] 选择    [Enter / 点击] 确认", "[Esc] 再開    [↑ / ↓] 選択    [Enter / クリック] 決定", "[Esc] 繼續    [↑ / ↓] 選擇    [Enter / 點擊] 確認"),
    ("[Esc] Pause", "[Esc] 暂停", "[Esc] 一時停止", "[Esc] 暫停"),
    ("Choose your language", "选择语言", "言語を選択", "選擇語言"),
    ("Confirm [Enter]", "确认 [Enter]", "決定 [Enter]", "確認 [Enter]"),
    ("[Up / Down] Select    [1 / 2 / 3 / 4] Language    [Enter] Confirm", "[↑ / ↓] 选择    [1 / 2 / 3 / 4] 语言    [Enter] 确认", "[↑ / ↓] 選択    [1 / 2 / 3 / 4] 言語    [Enter] 決定", "[↑ / ↓] 選擇    [1 / 2 / 3 / 4] 語言    [Enter] 確認"),
    ("[L] Change language", "[L] 更改语言", "[L] 言語を変更", "[L] 更改語言"),
    ("Convenience Store Night-Shift Horror — GitHub Game Off 2026 (Theme: SHIFT)", "便利店夜班恐怖游戏 — GitHub Game Off 2026（主题：SHIFT）", "コンビニ夜勤ホラー — GitHub Game Off 2026（テーマ：SHIFT）", "便利商店夜班恐怖遊戲 — GitHub Game Off 2026（主題：SHIFT）"),
    ("Complete clerk chores before timers expire. Serve every customer.", "在倒计时结束前完成店员工作，接待每一位顾客。", "制限時間内に店の仕事を終え、すべてのお客様に対応しましょう。", "在倒計時結束前完成店員工作，接待每一位顧客。"),
    ("When an entity reveals itself, the store SHIFTS into an abnormal state.", "当怪物显形时，便利店会 SHIFT 至异常状态。", "怪異が正体を現すと、店は異常な状態へ SHIFT します。", "當怪物顯形時，便利商店會 SHIFT 至異常狀態。"),
    ("Find the item it demands and deliver it to the Checkout Register to restore the store.", "找到它索要的商品并送到收银台，让便利店恢复正常。", "要求された商品を探してレジへ届けると、店が元に戻ります。", "找到它索要的商品並送到收銀臺，讓便利商店恢復正常。"),
    ("Controls: [WASD] Move   [Mouse] Look   [E / LMB] Interact / Hold Chore   [F / RMB] Throw Item   [G] Drop", "操作：[WASD] 移动  [鼠标] 视角  [E / 左键] 交互／长按工作  [F / 右键] 投掷  [G] 放下", "操作：[WASD] 移動  [マウス] 視点  [E / 左クリック] 調べる・長押しで作業  [F / 右クリック] 投げる  [G] 置く", "操作：[WASD] 移動  [滑鼠] 視角  [E / 左鍵] 交互／長按工作  [F / 右鍵] 投擲  [G] 放下"),
    ("Press [SPACE] or [E] to Start Shift", "按 [空格] 或 [E] 开始值班", "[Space] または [E] で勤務開始", "按 [空白鍵] 或 [E] 開始值班"),
    ("Customers Expected Tonight: %d   |   Auto-Save Active", "今晚顾客：{0} 位   |   自动存档已启用", "今夜のお客様：{0} 人   |   オートセーブ有効", "今晚顧客：{0} 位   |   自動存檔已啟用"),
    ("Press [SPACE] or [E] to Begin Night", "按 [空格] 或 [E] 开始夜班", "[Space] または [E] で夜勤開始", "按 [空白鍵] 或 [E] 開始夜班"),
    ("06:00 AM — DAWN", "06:00 — 天亮了", "午前06:00 — 夜明け", "06:00 — 天亮了"),
    ("%s COMPLETE — Store Handed Over to Day Shift", "{0}完成 — 已向白班交接", "{0} 終了 — 日勤に引き継ぎました", "{0}完成 — 已向白班交接"),
    ("Sales Revenue: $%.2f   |   Merchandise Losses: -$%.2f   |   Progress Saved", "销售收入：${0}   |   货损：-${1}   |   进度已保存", "売上：${0}   |   商品損失：-${1}   |   セーブ完了", "銷售收入：${0}   |   貨損：-${1}   |   進度已保存"),
    ("Press [SPACE] or [E] for Next Night", "按 [空格] 或 [E] 进入下一晚", "[Space] または [E] で次の夜へ", "按 [空白鍵] 或 [E] 進入下一晚"),
    ("SHIFT TERMINATED", "值班终止", "勤務終了", "值班終止"),
    ("Press [SPACE], [E], or [R] to Rewind to Start of Current Night", "按 [空格]、[E] 或 [R] 回到本晚开始时", "[Space]・[E]・[R] でこの夜の開始時点に戻る", "按 [空白鍵]、[E] 或 [R] 回到本晚開始時"),
    ("ALL NIGHT SHIFTS SURVIVED", "所有夜班已完成", "すべての夜勤を生き延びた", "所有夜班已完成"),
    ("The morning clerk unlocks the front door. The aisles are back where they belong.", "白班店员打开了店门，货架终于回到了原来的位置。", "朝の店員が入口の鍵を開ける。陳列棚は元の場所に戻っていた。", "白班店員打開了店門，貨架終於回到了原來的位置。"),
    ("Final Revenue: $%.2f   |   Total Losses: -$%.2f   |   Net: $%.2f", "总收入：${0}   |   总货损：-${1}   |   净收入：${2}", "総売上：${0}   |   総損失：-${1}   |   差引：${2}", "總收入：${0}   |   總貨損：-${1}   |   淨收入：${2}"),
    ("Thank you for playing SHIFT / 24! Press [SPACE] to Play Again.", "感谢游玩 SHIFT / 24！按 [空格] 重新开始。", "SHIFT / 24 を遊んでいただきありがとうございます！ [Space] でもう一度。", "感謝遊玩 SHIFT / 24！按 [空白鍵] 重新開始。"),
    ("%s  |  Customers: %d / %d  |  %s", "{0}  |  顾客：{1} / {2}  |  {3}", "{0}  |  接客：{1} / {2}  |  {3}", "{0}  |  顧客：{1} / {2}  |  {3}"),
    ("STATE: SHIFTED [ABNORMAL]", "状态：SHIFT［异常］", "状態：SHIFT［異常］", "狀態：SHIFT［異常］"),
    ("STATE: NORMAL", "状态：正常", "状態：正常", "狀態：正常"),
    ("SANITY: %d%%", "清醒度：{0}%", "正気度：{0}%", "清醒度：{0}%"),
    ("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f", "收入：+${0}  货损：-${1} / -${2}", "売上：+${0}  損失：-${1} / -${2}", "收入：+${0}  貨損：-${1} / -${2}"),
    ("CARRYING: %s  ([F/RMB] Throw  [G] Drop)", "手持：{0}（[F/右键] 投掷 [G] 放下）", "所持：{0}（[F/右クリック] 投げる [G] 置く）", "手持：{0}（[F/右鍵] 投擲 [G] 放下）"),
    ("HANDS: Empty  ([E] Pick up / Interact)", "空手（[E] 拾取／交互）", "何も持っていない（[E] 拾う・調べる）", "空手（[E] 拾取／交互）"),
    ("SHIFT CHECKLIST & ALERTS", "值班任务与提醒", "勤務チェックリスト・通知", "值班任務與提醒"),
    ("!! ABNORMAL ENTITY IN STORE !!", "！！店内出现异常顾客！！", "！！店内に怪異が出現！！", "！！店內出現異常顧客！！"),
    ("Demand: Find [%s] (Red Glow)", "要求：寻找［{0}］（红光）", "要求：［{0}］を探す（赤い光）", "要求：尋找［{0}］（紅光）"),
    ("-> Deliver it to the Checkout Register!", "→ 将商品送到收银台！", "→ 商品をレジへ届けよう！", "→ 將商品送到收銀臺！"),
    ("* Customer Help: Bring [%s]", "* 顾客求助：拿来［{0}］", "* お客様のお願い：［{0}］を持ってくる", "* 顧客求助：拿來［{0}］"),
    ("* Customer waiting at Checkout Register [E]", "* 顾客正在收银台等待 [E]", "* レジでお客様が待っています [E]", "* 顧客正在收銀臺等待 [E]"),
    ("* Customer in store (Watch them closely)", "* 顾客在店内（时刻留意）", "* お客様が店内にいます（目を離さない）", "* 顧客在店內（時刻留意）"),
    ("DAWN CLEANUP (Equip Mop in Warehouse):", "交班前打扫（去仓库拿拖把）：", "夜明け前の清掃（倉庫でモップを装備）：", "交班前打掃（去倉庫拿拖把）："),
    ("OVERDUE! (-Sanity)", "已超时！（持续扣清醒度）", "時間切れ！（正気度減少）", "已超時！（持續扣清醒度）"),
    ("%ds", "{0}秒", "{0}秒", "{0}秒"),
    ("Required", "必须完成", "必須", "必須完成"),
    ("- %s  [%s]", "- {0} ［{1}］", "- {0} ［{1}］", "- {0} ［{1}］"),
    ("NIGHT 1", "第一晚", "第1夜", "第一晚"),
    ("NIGHT 2", "第二晚", "第2夜", "第二晚"),
    ("NIGHT 3", "第三晚", "第3夜", "第三晚"),
    ("First night shift. Finish your chores, serve whoever walks in,\nclean up, and hand the store over at 06:00.", "第一次值夜班。完成店内工作，接待进店的每位顾客，\n然后打扫卫生，在 06:00 交班。", "初めての夜勤。店の仕事を終え、来店したお客様に対応し、\n清掃を済ませて06:00に引き継ごう。", "第一次值夜班。完成店內工作，接待進店的每位顧客，\n然後打掃衛生，在 06:00 交班。"),
    ("Note from the day shift: \"Some customers are not customers.\n If the lights go out, give them what they want. Don't let them touch you.\"", "白班的留言：“有些顾客并不是真正的顾客。\n如果灯灭了，就给它们想要的东西。别让它们碰到你。”", "日勤からのメモ：「お客様の中には、お客様ではないものがいる。\n明かりが消えたら、欲しがるものを渡すこと。触られないように。」", "白班的留言：「有些顧客並不是真正的顧客。\n如果燈滅了，就給它們想要的東西。別讓它們碰到你。」"),
    ("Last night of the week. The store keeps changing when they come.\nRemember where everything was.", "本周最后一次夜班。它们到来时，店内总会发生变化。\n记住每样东西原来的位置。", "今週最後の夜勤。やつらが来るたびに、店の様子が変わる。\nすべてがどこにあったか、覚えておこう。", "本週最後一次夜班。它們到來時，店內總會發生變化。\n記住每樣東西原來的位置。"),
    ("PURE Spring Water", "PURE 山泉水", "PURE 天然水", "PURE 山泉水"),
    ("DAILY Whole Milk", "DAILY 全脂牛奶", "DAILY 牛乳", "DAILY 全脂牛奶"),
    ("CITRA Sparkling Soda", "CITRA 汽水", "CITRA 炭酸飲料", "CITRA 汽水"),
    ("MIDNIGHT Noodle Cup", "MIDNIGHT 杯面", "MIDNIGHT カップ麺", "MIDNIGHT 杯麵"),
    ("CRISP Potato Chips", "CRISP 薯片", "CRISP ポテトチップス", "CRISP 薯片"),
    ("OAT Biscuits", "OAT 饼干", "OAT ビスケット", "OAT 餅乾"),
    ("PANTRY Tomato Soup", "PANTRY 番茄汤罐头", "PANTRY トマトスープ缶", "PANTRY 番茄湯罐頭"),
    ("MORNING Bread", "MORNING 面包", "MORNING パン", "MORNING 麵包"),
    ("SOFT Tissues", "SOFT 纸巾", "SOFT ティッシュ", "SOFT 面紙"),
    ("FRESH Rice Bowl", "FRESH 盒饭", "FRESH お弁当", "FRESH 便當"),
    ("UMAMI Soy Sauce", "UMAMI 酱油", "UMAMI しょうゆ", "UMAMI 醬油"),
    ("CLEAR Surface Cleaner", "CLEAR 清洁剂", "CLEAR 洗剤", "CLEAR 清潔劑"),
    ("Stock carton", "补货纸箱", "補充用段ボール", "補貨紙箱"),
    ("Mop", "拖把", "モップ", "拖把"),
    ("Restock Aisle 01", "补货：01 号货架", "01番棚の商品補充", "補貨：01 號貨架"),
    ("Restock Aisle 03", "补货：03 号货架", "03番棚の商品補充", "補貨：03 號貨架"),
    ("Restock wall shelf", "补货：靠墙货架", "壁際の棚の商品補充", "補貨：靠牆貨架"),
    ("Count cold storage", "清点冷柜货物", "冷蔵ケースの在庫確認", "盤點冷櫃貨物"),
    ("Count warehouse stock", "清点仓库货物", "倉庫の在庫確認", "盤點倉庫貨物"),
    ("Tidy Aisle 02", "整理 02 号货架", "02番棚の整理", "整理 02 號貨架"),
    ("Tidy drinks case", "整理饮品柜", "飲料ケースの整理", "整理飲品櫃"),
    ("Empty checkout bin", "清倒收银区垃圾", "レジ横のごみを捨てる", "清倒收銀區垃圾"),
    ("Empty restroom bin", "清倒卫生间垃圾", "トイレのごみを捨てる", "清倒洗手間垃圾"),
    ("Sign the shift log", "填写值班记录", "勤務日誌に記入する", "填寫值班記錄"),
    ("Take Stock Carton", "领取补货纸箱", "補充用段ボールを取る", "領取補貨紙箱"),
    ("Restock sold merchandise", "补齐已售商品", "売れた商品を補充する", "補齊已售商品"),
    ("Mop floor residue #%d", "清理地面污渍 #{0}", "床の汚れを拭く #{0}", "清理地面污漬 #{0}"),
    ("%s — Complete opening chores to let customers in.", "{0} — 完成开店任务后，顾客才会进店。", "{0} — 開店作業を終えると、お客様が来店します。", "{0} — 完成開店任務後，顧客才會進店。"),
    ("All chores complete. Ready for next customer.", "店内任务已完成，准备接待下一位顾客。", "作業完了。次のお客様を迎えましょう。", "店內任務已完成，準備接待下一位顧客。"),
    ("Chore completed! (%d remaining)", "任务完成！（还剩 {0} 项）", "作業完了！（残り{0}件）", "任務完成！（還剩 {0} 項）"),
    ("All customers served! Equip the Mop in the warehouse and clean the floor before 06:00.", "所有顾客已接待完毕！去仓库拿拖把，在 06:00 前清理地面。", "接客完了！倉庫でモップを装備し、06:00までに床を清掃しましょう。", "所有顧客已接待完畢！去倉庫拿拖把，在 06:00 前清理地面。"),
    ("Door chime: A customer has entered the store. Prioritize customer service!", "门铃响了，有顾客进店。请优先服务顾客！", "入店チャイム。お客様が来店しました。接客を優先しましょう！", "門鈴響了，有顧客進店。請優先服務顧客！"),
    ("Customer asks for help finding [%s]! Pick it up from a shelf and press [E] on them.", "顾客需要［{0}］！从货架拿取后，对准顾客按 [E]。", "お客様が［{0}］を探しています！棚から取り、お客様に向かって [E]。", "顧客需要［{0}］！從貨架拿取後，對準顧客按 [E]。"),
    ("Thanks! Heading to checkout.", "谢谢！我去收银台。", "ありがとう！レジへ行きます。", "謝謝！我去收銀臺。"),
    ("Customer is waiting at the Checkout Register. Press [E] to ring them up.", "顾客正在收银台等待，按 [E] 结账。", "お客様がレジでお待ちです。[E] でお会計。", "顧客正在收銀臺等待，按 [E] 結帳。"),
    ("Paid +$%.2f", "已付款 +${0}", "お支払い +${0}", "已付款 +${0}"),
    ("Sale complete (+$%.2f). The customer is leaving.", "结账完成（+${0}），顾客正在离店。", "お会計完了（+${0}）。お客様が退店します。", "結帳完成（+${0}），顧客正在離店。"),
    ("THE STORE SHIFTED! Find [%s] (Red Glow) and deliver it to the Checkout Register!", "便利店发生 SHIFT！找到［{0}］（红光），送到收银台！", "店が SHIFT した！［{0}］（赤い光）を探してレジへ！", "便利商店發生 SHIFT！找到［{0}］（紅光），送到收銀臺！"),
    ("Lights died behind you... An unseen entity is stalking! Find [%s] -> Checkout Register!", "身后的灯灭了……看不见的怪物正在靠近！找到［{0}］→ 收银台！", "背後の明かりが消えた……見えない怪異が迫る！［{0}］を探してレジへ！", "身後的燈滅了……看不見的怪物正在靠近！找到［{0}］→ 收銀臺！"),
    ("Abnormal customer appeased! Store shifting back to normal...", "异常顾客已满足，便利店正在恢复正常……", "怪異の要求を満たした！店が元に戻っていく……", "異常顧客已滿足，便利商店正在恢復正常……"),
    ("Direct hit! Entity stunned for %.1fs (-$%.2f cost loss)", "命中！怪物眩晕 {0} 秒（成本损失 -${1}）", "命中！怪異が{0}秒ひるんだ（原価損失 -${1}）", "命中！怪物眩暈 {0} 秒（成本損失 -${1}）"),
    ("Damaged %s (-$%.2f wholesale loss)", "损坏 {0}（成本损失 -${1}）", "{0}を破損（原価損失 -${1}）", "損壞 {0}（成本損失 -${1}）"),
    ("Store losses reached -$%.2f (Limit: -$%.0f). You were fired.", "店铺货损达 -${0}（上限 -${1}），你被解雇了。", "店の損失が -${0}（上限 -${1}）に達し、解雇された。", "店鋪貨損達 -${0}（上限 -${1}），你被解僱了。"),
    ("Your sanity collapsed during the night shift.", "夜班中，你的清醒度耗尽了。", "夜勤中に正気を失ってしまった。", "夜班中，你的清醒度耗盡了。"),
    ("Excuse me... where are the noodle cups?", "打扰一下……杯面在哪儿？", "すみません……カップ麺はどこですか？", "打擾一下……杯麵在哪兒？"),
    ("Do you have any bread left?", "还有面包吗？", "パンはまだありますか？", "還有麵包嗎？"),
    ("I... need something to drink.", "我……想喝点东西。", "何か……飲みたい。", "我……想喝點東西。"),
    ("Where do you keep the cleaner?", "清洁剂放在哪儿？", "洗剤はどこにありますか？", "清潔劑放在哪兒？"),
    ("Soy sauce. Where.", "酱油。在哪。", "しょうゆ。どこだ。", "醬油。在哪。"),
    ("Sorry, where's the water?", "不好意思，水在哪里？", "すみません、水はどこですか？", "不好意思，水在哪裡？"),
    ("Something warm... please.", "请给我……热的东西。", "温かいものを……ください。", "請給我……熱的東西。"),
    ("Can you find this for me?", "能帮我找一下这个吗？", "これを探してもらえますか？", "能幫我找一下這個嗎？"),
    ("[Need: %s]", "［需要：{0}］", "［探し物：{0}］", "［需要：{0}］"),
    ("WHERE IS MY %s", "我的 {0} 在哪", "私の{0}はどこだ", "我的 {0} 在哪"),
    ("[ STUNNED ]", "［眩晕中］", "［ひるんでいる］", "［眩暈中］"),
    ("BRING ME %s -> REGISTER", "把 {0} 送到收银台", "{0}をレジへ持ってこい", "把 {0} 送到收銀臺"),
    ("...Thank you.", "……谢谢。", "……ありがとう。", "……謝謝。"),
    ("[ Waiting at Register - Press E ]", "［收银台等待中 — 按 E］", "［レジで待機中 — E］", "［收銀臺等待中 — 按 E］"),
    ("[ GRABBED! Mash E or throw item! ]", "［被抓住了！连按 E 或用货物反击！］", "［捕まった！E連打か商品で反撃！］", "［被抓住了！連按 E 或用貨物反擊！］"),
    ("AMBUSHED BY UNSEEN ENTITY!", "遭到未目击怪物的偷袭！", "見えない怪異に襲われた！", "遭到未目擊怪物的偷襲！"),
    ("GRABBED! Hold/Mash [E] to struggle free, or [F/RMB] to smash with held item!", "被抓住了！长按／连按 [E] 挣脱，或按 [F/右键] 用手中货物反击！", "捕まった！[E] 長押し・連打で脱出、または [F/右クリック] で持っている商品をぶつける！", "被抓住了！長按／連按 [E] 掙脫，或按 [F/右鍵] 用手中貨物反擊！"),
    ("Smashed monster with merchandise! Escaped (Stunned %.1fs)", "用货物砸晕了怪物！成功挣脱（眩晕 {0} 秒）", "商品で怪異に反撃！脱出成功（{0}秒ひるむ）", "用貨物砸暈了怪物！成功掙脫（眩暈 {0} 秒）"),
    ("GRABBED! Hold/Mash [E] to struggle (%d%%)%s", "被抓住了！长按／连按 [E] 挣脱（{0}%）{1}", "捕まった！[E] 長押し・連打で脱出（{0}%）{1}", "被抓住了！長按／連按 [E] 掙脫（{0}%）{1}"),
    (" | [F/RMB] Smash with item!", " | [F/右键] 用货物反击！", " | [F/右クリック] 商品で反撃！", " | [F/右鍵] 用貨物反擊！"),
    ("Broke free! Keep moving!", "挣脱成功！继续跑！", "振りほどいた！走り続けろ！", "掙脫成功！繼續跑！"),
    ("[E] Take Stock Carton", "[E] 领取补货纸箱", "[E] 補充用段ボールを取る", "[E] 領取補貨紙箱"),
    ("Stock Carton (from Warehouse)", "补货纸箱（仓库）", "補充用段ボール（倉庫）", "補貨紙箱（倉庫）"),
    ("Mop (from Warehouse)", "拖把（仓库）", "モップ（倉庫）", "拖把（倉庫）"),
    ("%s — Requires %s", "{0} — 需要 {1}", "{0} — {1}が必要", "{0} — 需要 {1}"),
    ("[Hold E] %s (%d%%)", "[长按 E] {0}（{1}%）", "[E 長押し] {0}（{1}%）", "[長按 E] {0}（{1}%）"),
    ("[E] Hand %s to Customer", "[E] 将 {0} 交给顾客", "[E] {0}をお客様に渡す", "[E] 將 {0} 交給顧客"),
    ("Customer: \"%s\" (Bring %s)", "顾客：“{0}”（拿来 {1}）", "お客様：「{0}」（{1}を持ってくる）", "顧客：「{0}」（拿來 {1}）"),
    ("[E] Ring Up Customer at Checkout", "[E] 为收银台的顾客结账", "[E] レジのお客様のお会計", "[E] 為收銀臺的顧客結帳"),
    ("ENTITY DEMANDS: %s (Deliver to Checkout Register!)", "怪物要求：{0}（送到收银台！）", "怪異の要求：{0}（レジへ届ける！）", "怪物要求：{0}（送到收銀臺！）"),
    ("[E] Deliver %s to appease the entity!", "[E] 交出 {0}，满足怪物的要求！", "[E] {0}を渡して怪異を鎮める！", "[E] 交出 {0}，滿足怪物的要求！"),
    ("Checkout: Bring [%s] here to banish the entity", "收银台：将［{0}］带到这里，送走怪物", "レジ：［{0}］をここに運び、怪異を追い払う", "收銀臺：將［{0}］帶到這裡，送走怪物"),
    ("[E] Ring Up Customer", "[E] 为顾客结账", "[E] お会計", "[E] 為顧客結帳"),
    ("[E] Take Hot Oden Cup (Strong Stun Throwable)", "[E] 领取热关东煮杯（可投掷眩晕怪物）", "[E] 熱いおでんを取る（投げると怪異がひるむ）", "[E] 領取熱關東煮杯（可投擲眩暈怪物）"),
    ("[E] Pick up %s", "[E] 拾取 {0}", "[E] {0}を拾う", "[E] 拾取 {0}"),
    ("[E] Equip Mop", "[E] 拿起拖把", "[E] モップを装備する", "[E] 拿起拖把"),
    ("[E] Take %s from shelf", "[E] 从货架拿取 {0}", "[E] 棚から{0}を取る", "[E] 從貨架拿取 {0}"),
    ("[SPACE / E] New Game — Night 1", "[空格 / E] 新游戏 — 第一晚", "[Space / E] はじめから — 第1夜", "[空白鍵 / E] 新遊戲 — 第一晚"),
    ("[C] Continue Saved Game — %s", "[C] 继续存档 — {0}", "[C] つづきから — {0}", "[C] 繼續存檔 — {0}"),
    ("Chore time limit: %ds. Timers are paused on this screen.", "每项任务 {0} 秒，本说明页不计时。", "作業ごとに{0}秒。この画面では時間は進みません。", "每項任務 {0} 秒，本說明頁不計時。"),
    ("FOLLOW AISLE", "沿通道前进", "通路を進む", "沿通道前進"),
    ("WAREHOUSE CARTON", "仓库：补货纸箱", "倉庫：補充用段ボール", "倉庫：補貨紙箱"),
    ("MOP", "拖把", "モップ", "拖把"),
    ("WORK HERE", "在此工作", "ここで作業", "在此工作"),
    ("%s · %dm", "{0} · {1}米", "{0} · {1}m", "{0} · {1}公尺"),
    ("NEXT CHORE: %s", "当前任务：{0}", "次の作業：{0}", "目前任務：{0}"),
    ("1/2  Warehouse: press [E] to take a stock carton.", "1/2  前往仓库，按 [E] 领取补货纸箱。", "1/2  倉庫へ行き、[E] で補充用段ボールを取る。", "1/2  前往倉庫，按 [E] 領取補貨紙箱。"),
    ("2/2  Return to the marked shelf; HOLD [E] to restock.", "2/2  返回标记的货架，长按 [E] 补货。", "2/2  印のある棚に戻り、[E] 長押しで補充する。", "2/2  返回標記的貨架，長按 [E] 補貨。"),
    ("1/2  Follow the marker; press [E] to take the mop.", "1/2  跟随标记，按 [E] 拿起拖把。", "1/2  マーカーに従い、[E] でモップを取る。", "1/2  跟隨標記，按 [E] 拿起拖把。"),
    ("2/2  Return to the marked spill; HOLD [E] to mop.", "2/2  前往标记的污渍，长按 [E] 清理。", "2/2  印のある汚れに向かい、[E] 長押しで拭く。", "2/2  前往標記的污漬，長按 [E] 清理。"),
    ("Follow the yellow aisle markers to the work location.", "跟随黄色通道标记，前往任务位置。", "黄色い通路マーカーに従い、作業場所へ向かう。", "跟隨黃色通道標記，前往任務位置。"),
    ("Look at the task marker and HOLD [E] to work.", "对准任务标记，长按 [E] 完成工作。", "作業マーカーに視点を合わせ、[E] を長押し。", "對準任務標記，長按 [E] 完成工作。"),
    ("[TAB] Next chore   |   Finish all chores to admit customers.", "[TAB] 切换任务  |  全部完成后，顾客才会进店。", "[Tab] 次の作業  |  全作業を終えると、お客様が来店します。", "[TAB] 切換任務  |  全部完成後，顧客才會進店。"),
]

ENTRIES += [
    ("Prepare hot oden", "煮关东煮", "おでんを仕込む", "煮關東煮"),
    ("Next chore: %s. Follow the yellow marker.", "下一项工作：{0}，请跟随黄色标记。", "次の作業：{0}。黄色いマーカーに従いましょう。", "下一項工作：{0}，請跟隨黃色標記。"),
    ("At the oden station, HOLD [E] to prepare the broth.", "在关东煮台前长按 [E]，准备热汤。", "おでん台で [E] を長押しして、だしを準備する。", "在關東煮臺前長按 [E]，準備熱湯。"),
]

ENTRIES += [
    ("Hands full. Put it back on a shelf, or press [G] to drop it.", "手里已有物品。放回货架，或按 [G] 放下。", "手がふさがっています。棚に戻すか、[G] で置きましょう。", "手上已有物品。放回貨架，或按 [G] 放下。"),
    ("[E] Put back %s", "[E] 放回 {0}", "[E] {0} を戻す", "[E] 放回 {0}"),
    ("Tidy up: %d items left on the floor", "整理：地上还有 {0} 件商品", "片付け：床に商品が {0} 個残っています", "整理：地上還有 {0} 件商品"),
    ("Pick up loose merchandise and press [E] at any shelf to put it back.", "捡起散落的商品，对准任意货架按 [E] 放回。", "散らばった商品を拾い、棚に向かって [E] で戻す。", "撿起散落的商品，對準任意貨架按 [E] 放回。"),
    ("Chores done! Put the loose merchandise back on the shelves (%d left).", "工作完成！请把散落的商品放回货架（还剩 {0} 件）。", "作業完了！散らばった商品を棚に戻しましょう（残り {0} 個）。", "工作完成！請把散落的商品放回貨架（還剩 {0} 件）。"),
]
ENTRIES += [
    ("Reset warped welcome sign", "复位被篡改的迎宾牌", "歪んだ歓迎看板を戻す", "復位被篡改的迎賓牌"),
    ("Push shifted Aisle 02 gondola back", "推回暗移的二号货架", "ずれた2番棚を押し戻す", "推回暗移的二號貨架"),
    ("Close rattling cooler & check stock", "复位异响冷柜并核对商品", "鳴動する冷蔵棚を戻し在庫確認", "復位異響冷櫃並核對商品"),
    ("...So thirsty for thick red tomato soup in a tin can...", "……低语：想喝铁罐里的红色浓汤……", "……赤い缶のトマトスープが欲しい……", "……低語：想喝鐵罐裡的紅色濃湯……"),
    ("...White and cold from the dairy carton... not dark...", "……低语：要冰柜里白色的鲜奶……不是黑的……", "……冷蔵棚の白いミルク……黒いのではない……", "……低語：要冰櫃裡白色的鮮奶……不是黑的……"),
    ("...Soft sliced loaf from the morning bakery... no broth...", "……低语：要早上的切片面包……不要汤……", "……朝の柔らかい食パン……汁はいらない……", "……低語：要早上的切片麵包……不要湯……"),
    ("Emergency Breaker", "应急电闸", "非常用ブレーカー", "應急電閘"),
    ("Emergency Breaker (Standby — Use during Store Shift)", "应急电闸（待命——店铺异变熄灯时可用）", "非常用ブレーカー（待機中 — 異変時に使用）", "應急電閘（待命——店鋪異變熄燈時可用）"),
    ("Emergency Breaker active (%ds left)", "应急电闸强制照明中（剩余 {0} 秒）", "非常用ブレーカー作動中（残り {0} 秒）", "應急電閘強制照明中（剩餘 {0} 秒）"),
    ("Emergency Breaker recharging (%ds)", "应急电闸冷却中（{0} 秒）", "非常用ブレーカー冷却中（{0} 秒）", "應急電閘冷卻中（{0} 秒）"),
    ("[E] Flip Emergency Breaker (Restore lights 10s - Attracts Entity!)", "[E] 拉下应急电闸（恢复照明与布局 10 秒 — 会吸引怪物！）", "[E] 非常用ブレーカーを入れる（10秒復旧 — 怪異を引き寄せる！）", "[E] 拉下應急電閘（恢復照明與佈局 10 秒 — 會吸引怪物！）"),
    ("Customer: \"%s\" (Bring %s) | %s", "顾客：“{0}”（表面要 {1}） | {2}", "客：「{0}」（表向き：{1}） | {2}", "顧客：「{0}」（表面要 {1}） | {2}"),
    ("MICRO-SHIFT ANOMALY: %s! Reset it with [Hold E]!", "局部异变：{0}！请前往长按 [E] 复位！", "局所異変：{0}！[E] 長押しで戻せ！", "局部異變：{0}！請前往長按 [E] 復位！"),
    ("Emergency Breaker pulled! Normal lights & layout restored for 10s — The noise drew the entity toward the corridor!", "已拉下应急电闸！照明与货架恢复 10 秒——噪音正把怪物引向后勤走廊！", "ブレーカー作動！10秒間照明と棚が復旧 — 音で怪異が通路へ接近中！", "已拉下應急電閘！照明與貨架恢復 10 秒——噪音正把怪物引向後勤走廊！"),
    ("...You knew what I truly wanted.", "……你竟然看穿了我真正想要的东西。", "……本当に欲しいものが分かったのか。", "……你竟然看穿了我真正想要的東西。"),
    ("Disguise seen through! Appeased before the Shift (+$10.00 bonus, +Sanity).", "识破伪装！在店铺异变前平息了怪客（+$10.00 奖金，恢复清醒度）。", "偽装を見破った！異変前に退去させた（+$10.00 ボーナス・正気度回復）。", "識破偽裝！在店鋪異變前平息了怪客（+$10.00 獎金，恢復清醒度）。"),
    ("Shelves swapped stock! Corridor Breaker [E] restores light 10s.", "货架商品已错位互换！后勤走廊电闸 [E] 可恢复照明 10 秒。", "棚の在庫が入れ替わった！通路のブレーカー [E] で10秒照明復旧。", "貨架商品已錯位互換！後勤走廊電閘 [E] 可恢復照明 10 秒。"),
    ("Suspect disguise? Hand their whispered true item to ward off!", "怀疑是伪装？递交其低语暗示的真正商品可直接驱离！", "偽装の疑い？呟いている本当の品を渡せば撃退できる！", "懷疑是偽裝？遞交其低語暗示的真正商品可直接驅離！"),
    ("! ANOMALY: %s (%ds)", "！局部异变：{0}（{1}秒）", "！局所異変：{0}（{1}秒）", "！局部異變：{0}（{1}秒）"),
    ("Follow the yellow marker; HOLD [E] to reset the store anomaly.", "跟随黄色标记，长按 [E] 复位店铺局部异变。", "黄色いマーカーに従い、[E] 長押しで異変を戻す。", "跟隨黃色標記，長按 [E] 復位店鋪局部異變。"),
]

PRINTF = re.compile(r"%%|%[-+0 #]*\d*(?:\.\d+)?[sdf]")


def english_template(source):
    index = 0

    def replace(match):
        nonlocal index
        if match[0] == "%%":
            return "%"
        placeholder = "{" + str(index) + "}"
        index += 1
        return placeholder

    return PRINTF.sub(replace, source)


def build():
    packs = {"en": {}, "ja": {}, "zh-Hant": {}, "zh-Hans": {}}
    for source, chinese, japanese, traditional_chinese in ENTRIES:
        if source in packs["en"]:
            raise ValueError(f"Duplicate source: {source}")
        packs["en"][source] = english_template(source)
        packs["zh-Hans"][source] = chinese
        packs["ja"][source] = japanese
        packs["zh-Hant"][source] = traditional_chinese
    dest = ROOT / "Content/Localization/Strings"
    dest.mkdir(parents=True, exist_ok=True)
    for code, strings in packs.items():
        (dest / f"{code}.json").write_text(json.dumps(strings, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(ENTRIES)} entries in each of {len(packs)} language packs")


if __name__ == "__main__":
    build()
