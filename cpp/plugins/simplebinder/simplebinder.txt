TITLE: SimpleBinder解説
AUTHOR: miahmie

●これは何か

吉里吉里のプラグインを比較的簡単に書けるようにするヘルパです。
似たようなものにncbindがありますが，それを簡易にしたバージョンになっています。

ncbindとの比較：
・自動型変換がない
・関数の可変長引数に対応
・登録のための記述方法がシンプル

なぜncbindを使わないか：
・もっと手軽にプラグインを書きたい
・可変長引数を手軽に扱える（ncbindの場合RawCallbackでしか扱えない）
・ncbindのコンパイルエラーが複雑すぎて追うのが面倒
・ncbindを書いたのが古すぎて今見ると（以下略


以下，吉里吉里のプラグインの基本的な知識があることを前提として説明します。


●使い方

#include <windows.h>
#include "tp_stub.h"
#include "simplebinder.hpp"

などとして予めtp_stub等をインクルードの上で
simplebinder.hppをインクルードします。


●記述方法

使うのは SimpleBinder::BindUtil の１種類のみです。
使う前に予め

using SimpleBinder::BindUtil;

などとしておくと良いでしょう。
プラグイン登録／解除（V2Link/V2Unlink）の所で，

BindUtil(link)
 .Function(name, method)
 .Property(name, getter, setter)
 .Constant(name, value)
 .Variant(name, value);

のように数珠繋ぎで記入することができます。
linkはtrueなら登録，falseなら解除（消す）の動作になります。
その他の詳細説明は後述します。


既存のクラスオブジェクトにメンバを生やしたい場合は，

BindUtil(basename, link)

のように，登録基点となる位置（例：basename=TJS_W("System")等）を指定すると
そこに対して登録／解除を行います。

BindUtil(TJS_W("Hoge.Fuga.Foo.Bar"), link)

のように，深い階層のオブジェクトに対しても操作可能です。
（ただし，その階層が既に存在していることが前提です）


iTJSDispatch2*のディスパッチに対しても操作可能です。

BindUtil(object, link)

のようにしてください。


ネイティブクラスも登録可能ですが，複数のクラスを登録する場合は
数珠繋ぎを切ってから再度別のクラスを登録する形にします。

BindUtil(link)
 .Class(classnameA, factory)
 .Function(name, method);

BindUtil(link)
 .Class(classnameB, factory)
 .Function(name, method);

これは.Classを使った場合にそれ以降の登録対象がクラスオブジェクトになるためです。


●登録できる関数

BindUtilでは下記の形式の関数のみ登録可能です。（いずれもN=8まで対応）

・method(result, args*)型

tjs_error (*)(tTJSVariant *result);
tjs_error (*)(tTJSVariant *result, tTJSVariant *arg1);
tjs_error (*)(tTJSVariant *result, tTJSVariant *arg1, tTJSVariant *arg2);
tjs_error (*)(tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN);

・method(result, args*, optnum, optargs)型

tjs_error (*)(tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN, tjs_int optnum, tTJSVariant **optargs);
※上と同様にarg1～argNの個数は可変です

・method(objthis, result, args*)型

tjs_error (*)(iTJSDispatch2 *objthis, tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN);

・method(objthis, result, args*, optnum, optargs)型

tjs_error (*)(iTJSDispatch2 *objthis, tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN, tjs_int optnum, tTJSVariant **optargs);


・instance->method(...)型（※クラス登録時のみ）

tjs_error (Class::*)(tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN);
tjs_error (Class::*)(tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN, tjs_int optnum, tTJSVariant **optargs);

・method(instance, ...)型（※クラス登録時のみ）

tjs_error (*)(Class *instance, tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN);
tjs_error (*)(Class *instance, tTJSVariant *result, tTJSVariant *arg1, ... tTJSVariant *argN, tjs_int optnum, tTJSVariant **optargs);


TJSから呼ばれる際，arg1～argNの個数に満たない場合は
対象関数が呼ばれる前にTJS_E_BADPARAMCOUNTが返ります。
可変長引数向けのoptnumはarg1～argNの個数を差し引いた数値が渡されます。
optargsの有効個数として扱うことが出来ます。（追加引数がなければoptnum=0）


●登録できるプロパティ

プロパティのsetter,getter向け関数は下記の形式で登録可能です。

・getter型

tjs_error (*)(tTJSVariant *result);
tjs_error (*)(iTJSDispatch2 *objthis, tTJSVariant *result);

tjs_error (Class::*)(tTJSVariant *result) const; //※クラス登録時のみ
tjs_error (Class::*)(tTJSVariant *result); //※クラス登録時のみ
tjs_error (*)(const Class *instance, tTJSVariant *result); //※クラス登録時のみ
tjs_error (*)(Class *instance, tTJSVariant *result); //※クラス登録時のみ


・setter型

tjs_error (*)(const tTJSVariant *value);
tjs_error (*)(iTJSDispatch2 *objthis, const tTJSVariant *value);

tjs_error (Class::*)(const tTJSVariant *value); //※クラス登録時のみ
tjs_error (*)(Class *instance, const tTJSVariant *value); //※クラス登録時のみ


●登録できるコンストラクタ／デストラクタ

クラスを登録する際のコンストラクタ／デストラクタは下記の形式で登録可能です。

・factory型

tjs_error (*)(Class* &instance, tTJSVariant *arg1, ... tTJSVariant *argN);
tjs_error (*)(Class* &instance, tTJSVariant *arg1, ... tTJSVariant *argN, tjs_int optnum, tTJSVariant **optargs);

tjs_error (*)(iTJSDispatch2 *objthis, Class* &instance, tTJSVariant *arg1, ... tTJSVariant *argN);
tjs_error (*)(iTJSDispatch2 *objthis, Class* &instance, tTJSVariant *arg1, ... tTJSVariant *argN, tjs_int optnum, tTJSVariant **optargs);

・terminator型

void (*)(Class *instance);


●v2link.cppについて

BindUtilはv2link.cppとセットで使うことを前提として設計されています。
v2link.cpp は最低限のV2Link/V2Unlinkを定義したソースで

extern bool onV2Link();
extern bool onV2Unlink();

を外部に置くことで，TJSへの任意の登録解除操作を行います。
下記のように定義することで，登録・解除を１関数で行うことができます。

bool Entry(bool link) {
  return 
	(BindUtil(link)
	 .Function(TJS_W("Hoge"), &Hoge)
	 .Property(TJS_W("Fuga"), &FugaGet, &FugaSet)
	 .IsValid()
	);
}

bool onV2Link()   { return Entry(true);  }
bool onV2Unlink() { return Entry(false); }


●ncbindとの共有について

単体のFunction/Property/Constant/Variantに関しては
ncbindと混ぜて使うことが出来ます。
{PRE/POST}_*_CALLBACK のマクロで登録した関数内で
BindUtilの登録／解除操作を行ってください。
なお，ncbindを使うときは，前述の v2link.cpp は使用できません。
（ncbind.cppにV2Link/V2Unlinkが存在するため）

クラスの登録に関しては，ネイティブクラス登録時のID管理の機構が違うため，
混ぜて使うことはできません。

・ncbindで登録したクラスAに，BindUtilで関数を追加する
・BindUtilで登録したクラスBに，ncbindで関数を追加する

といったようなことは基本的には出来ません。
自前でiTJSDispatch2*とインスタンスのポインタの変換を行うなどすれば
できなくはありませんが，混乱の元になりますのでお勧めしません。

完全に独立したクラス同士であれば，混ぜて運用することは可能です。

・ncbindで登録するクラスAと，BindUtilで登録するクラスB

ただしこの場合，クラスBに関してncbindでの引数の自動変換はできなくなります。



●リファレンス

※分かりやすくするため，一部template<～>については記述を省略しています。

class BindUtil {
public:
  /**
   * コンストラクタ
   * @param link リンク／解除指定
   * @param base 基点階層（"."で区切って階層指定可能）
   * @param store 基点オブジェクト（指定がない場合はglobal）
   */
  BindUtil(bool link); //< global基点
  BindUtil(const ttstr &base, bool link); //< 指定階層基点
  BindUtil(iTJSDispatch2 *store, bool link); //< 指定object基点
  BindUtil(const ttstr &base, iTJSDispatch2 *store, bool link); //< 組み合わせ

  /**
   * 関数を登録
   * @param name 登録関数名
   * @param method 登録対象のmethod型の関数
   */
  BindUtil& Function(const ttstr &name, const Method &method);

  /**
   * プロパティを登録
   * @param name 登録プロパティ名
   * @param get 登録対象のgetter型の関数
   * @param set 登録対象のsetter型の関数
   * ※setter/getterどちらかが無い場合は 0 を指定します
   */
  BindUtil& Property(const ttstr &name, const Getter &get, const Setter &set);
  BindUtil& Property(const ttstr &name, const Getter &get, int noset);
  BindUtil& Property(const ttstr &name, int noget, const Setter &set);

  /**
   * 定数を登録
   * @param name 登録定数名
   * @param value 登録対象の値(tTJSVariantのコンストラクタに渡せる型)
   * ※valueを返すgetterのみのプロパティを登録します
   */
  BindUtil& Constant(const ttstr &name, const Value &value);

  /**
   * tTJSVariantを登録
   * @param name 登録定数名
   * @param value 登録対象の値(tTJSVariantのコンストラクタに渡せる型)
   * @param flag PropSetに渡すフラグ値
   */
  BindUtil& Variant(const ttstr &name, const Value &value, tjs_uint32 flag=TJS_MEMBERENSURE|TJS_IGNOREPROP);

  /**
   * クラスをを登録
   * @param name 登録クラス名
   * @param ctor 登録対象のfactory型の関数
   * @param dtor 登録対象のterminator型の関数
   * ※dtorが指定されない場合は delete instance されます
   */
  BindUtil& Class(const ttstr &name, const Factory& ctor);
  BindUtil& Class(const ttstr &name, const Factory& ctor, const Terminator& dtor);

  /**
   * 以降のFunction/Property登録の関数のコンテキストを変更
   * @param context コンテキスト
   * ※既存のSingletonなクラスオブジェクトに関数やプロパティを差し込む場合に
   *   コンテキストからthisを取得するなど特殊な目的で使用します
   */
  BindUtil& SetContext(iTJSDispatch2 *context = 0);

  /**
   * エラーチェック
   * 今までの登録／解除動作でエラーがなかったかどうか確認
   * @return valid エラーが無い場合はtrue
   */
  bool IsValid() const;

  /**
   * iTJSDispatch2から任意のクラスインスタンスの変換
   * @param obj 変換対象オブジェクト
   * @param instance 変換したいクラスの型ポインタ（NULLで構いません）
   * @return ポインタ
   */
  template <class Class>
  static Class* GetInstance(iTJSDispatch2 *obj, Class *instance);

  /**
   * 任意階層にあるオブジェクトを取得
   * @param base 基点階層（"."で区切って階層指定可能）
   * @param store 基点オブジェクト（省略時orNULLの場合はglobal）
   * @return オブジェクトがあればiTJSDispatch2ポインタ，なければNULL
   */
  static iTJSDispatch2* GetObject(const ttstr &base, iTJSDispatch2 *root = 0);
};


●ライセンスについて

simplebinder.hpp, v2link.cppとも
吉里吉里２／吉里吉里Ｚのライセンス準拠で扱ってください。

