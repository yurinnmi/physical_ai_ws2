#!/bin/bash
# Physical AI Demo 起動スクリプト
# 仮想環境を有効化して main.py を実行し、終了後に仮想環境を無効化する。

SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
cd "$SCRIPT_DIR"

source .venv/bin/activate

python main.py

deactivate

echo ""
read -n 1 -s -r -p "終了しました。何かキーを押すとウィンドウを閉じます..."
echo ""
