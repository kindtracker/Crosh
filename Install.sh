#!/bin/sh

set -e

PREFIX="${PREFIX:-/usr/local}"
INSTALL_DIR="$PREFIX/bin"
CROSH_DIR="."

mkdir -p "$INSTALL_DIR"
cp "$CROSH_DIR/Crosh.lua" "$INSTALL_DIR/crosh.lua"
cp "$CROSH_DIR/CroshRuntime.so" "$INSTALL_DIR/CroshRuntime.so"

cat >"$INSTALL_DIR/crosh" <<EOF
#!/bin/sh
exec luajit "$INSTALL_DIR/crosh.lua" "\$@"
EOF

chmod +x "$INSTALL_DIR/crosh"

echo "Crosh installed to $INSTALL_DIR"
