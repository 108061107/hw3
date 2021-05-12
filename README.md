# hw3
首先開始兩個thread function，分別為gesture UI mode和tilt angle detection mode，來完成RPC loop的操作。\
在RPC loop裡面接受RPC的指令，將LED1打開，代表進入gesture UI mode: \
在這個mode裡面依照手勢選擇threshold angle，選擇期間顯示目前的threshold angle在uLCD上，選擇完畢後按下userbottom，顯示選擇好的threshold angle在screen上，並publish選擇好的threshold angle給python端，\
當python端接收到此訊息，則傳送rpc指令將LED1關閉，停止gesture UI mode並回到RPC loop等待下一個RPC指令。\
使用RPC指令開啟LED2，則進入tilt angle detection mode: \
一開始先閃爍LED3 5秒，代表正在測量參考的重力加速度值，之後開始偵測角度。\
將偵測到的角度顯示在screen和uLCD上，若偵測到超過設定好的threshold angle的角度，publish "overtilt"的告知訊息和偵測到的角度到python端，\
當超過角度的次數超過5次，python端傳送RPC指令將LED2關閉，結束tilt angle detection mode並回到RPC loop等待下一個RPC指令。\

