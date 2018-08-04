# chat-relay
Raspberry Pi を使用してHipChatルームにメッセージがあったらパトランプを点滅させる

HipChatのアクセストークンを設定する
#define ACCESS_TOKEN	"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"


![image](https://user-images.githubusercontent.com/12773136/43672594-4e74b9e2-97ec-11e8-9ac2-3d44d97ed0a0.jpg)


# memo

http://qiita.com/edo_m18/items/41770cba5c166f276a83
char msg[1000]にする


#include <openssl/ssl.h>をインクルードするため
http://raspberrypi.stackexchange.com/questions/33597/cant-apt-get-install-libssl-dev
apt-get update
apt-get upgrade
apt-get install libssl-dev 

----------------------

https://api.hipchat.com/v1/rooms/history?room_id=yokochi-test&date=2017-08-20&timezone=JST&format=json&auth_token=846d0ae0180e6c1c0a6e183b9af650



{"error":{"code":403,"type":"Forbidden","message":"You have exceeded the rate limit. See https:\/\/www.hipchat.com\/docs\/api\/rate_limiting"}}


https://developer.atlassian.com/hipchat/guide/hipchat-rest-api/api-rate-limits


APIレート制限
あなたのアドオンは5分間に500のAPIリクエストを作成できます。制限を超過する   と、HTTPステータス429と、制限されたというメッセージが表示されます。
さらに、ルームや個人にメッセージを送信するAPIメソッドは、ルーム「スパム」を防ぐ手段として、毎分30リクエストに制限されています。この制限は、部屋と人に固有です。グループに送信されたメッセージは、グループ内の他の受信者の制限値にはカウントされません。
