#!/bin/bash
echo "QT=>" "$QT" "<="
#!/bin/bash
if [ "$QT_VER" = "5.5.1" ]
then
 brew install file://$PWD/qt5.rb
else
 brew install $QT
fi
