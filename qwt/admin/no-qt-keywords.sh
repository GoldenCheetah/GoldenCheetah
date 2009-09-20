#! /bin/sh

find src -name "qwt_*.h" | xargs grep -l 'signals:' | xargs sed -i "s/signals:/Q_SIGNALS:/"
find src -name "qwt_*.h" | xargs grep -l 'slots:' | xargs sed -i "s/signals:/Q_SLOTS:/"
find src -name "qwt_*.cpp" | xargs grep -l 'emit ' | xargs sed -i "s/emit /Q_EMIT /"

echo "CONFIG += no_keywords" >> src/src.pro


