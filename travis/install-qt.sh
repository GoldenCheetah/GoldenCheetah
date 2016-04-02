#!/bin/bash
echo "QT=>" "$QT" "<="
#!/bin/bash
if [ "$QT_VER" = "5.5.1" ]
then
 brew install https://raw.githubusercontent.com/Homebrew/homebrew/fb64f6cd91ff19d9c651b7851a74808bde6bc60f/Library/Formula/qt5.rb
else
 brew install $QT
fi
