## Darts-clone: A clone of Darts (Double-ARray Trie System) ##

Darts-clone is a clone of Darts (Double-ARray Trie System), which is a C++ header library for double-array structure.

The major advantages of Darts-clone are as follows:

1. Half-size units

> While Darts allocates 8 bytes to each unit, Darts-clone allocates only 4 bytes to each unit. This feature simply halves the size of dictionaries.

2. More sophisticated structure

> While Darts uses a trie to implement a dictionary, Darts-clone uses a Directed Acyclic Word Graph (DAWG), which is derived from a trie by merging its common subtrees. Darts-clone thus requires less units than Darts if a given keyset contains many duplicate values.

Due to these advantages, Darts-clone achieves more compact dictionaries without degrading search performance.

  * Project URL: http://code.google.com/p/darts-clone/
  * Documentation: [Introduction](Introduction.md), [ClassInterface](ClassInterface.md)


---


## Darts-clone: Darts（Double-ARay Trie System）のクローン ##

Darts-clone はダブル配列の C++ ヘッダライブラリである Darts のクローンであり，以下のような特長を持っています．

1. 要素のサイズが半分

> Darts が 8 bytes の要素を用いるのに対し，Darts-clone は 4 bytes の要素を使用します．そのため，辞書のサイズは単純に半減します．

2. 辞書データ構造の改良

> Darts がトライというデータ構造を用いるの対し，Darts-clone は Directed Acyclic Word Graph（DAWG）というデータ構造を用います．そして，DAWG はトライに含まれる共通部分を併合することで得られるデータ構造なので，キーに関連付ける値に重複がたくさんあれば，Darts-clone の方が少ない要素で辞書を構成できます．

これらの利点により，Darts-clone は検索の機能や時間を劣化させず，よりコンパクトな辞書を実現しています．

  * プロジェクト URL: http://code.google.com/p/darts-clone/
  * ドキュメント: [Introduction](Introduction.md), [ClassInterface](ClassInterface.md)