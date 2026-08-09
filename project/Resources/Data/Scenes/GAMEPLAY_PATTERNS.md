# ゲームプレイ配置パターン

`GAMEPLAY_objects.json` が既定配置です。追加パターンは、同じフォルダーへ
`GAMEPLAY_<パターンID>_objects.json` という名前で保存すると自動検出されます。

パターンIDに使用できる文字は、半角英数字・`-`・`_` です。

## エディターで増やす

1. Editor Toolbar の `Gameplay Stage Pattern` から元にする配置を選びます。
2. ゲームプレイ中なら `Load Selected Pattern` で読み込みます。
3. `new pattern id` に新しいIDを入力します。
4. `Duplicate Current Pattern` を押すと、現在の配置を新しいJSONへ複製します。
5. 通常どおり配置を編集し、Hierarchy の `Save Layout` で保存します。

通常のゲーム開始時は、プレイヤー決定後のステージ選択画面に自動検出されたパターンが表示されます。
そこで決定したパターンを読み込んでゲームプレイを開始します。

## 差分だけで作る

既定配置を継承し、変更したいオブジェクトだけを `name` で指定できます。

```json
{
    "extends": "GAMEPLAY_objects.json",
    "objectOverrides": [
        {
            "name": "terrain_7",
            "transform": {
                "scale": [1.5, 1.0, 1.5]
            }
        }
    ]
}
```

- `objectOverrides`: 既存オブジェクトを `name` で探し、指定した項目だけ上書きします。
- `objectAdditions`: 配列に書いた新規オブジェクトをルートへ追加します。
- 継承は最大8段までです。`extends` には同じフォルダー内のJSONファイル名だけを指定できます。
- 差分形式のパターンをエディターの `Save Layout` で保存すると、その時点の完全な配置JSONへ展開されます。

サンプルとして `wide`（広域型）と `rush`（高密度スポーン型）が入っています。
