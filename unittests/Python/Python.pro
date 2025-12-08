TEMPLATE = aux
TARGET = PythonTests

# Only define the check target, no build output needed for python scripts
check.commands = python3 $$PWD/test_api.py
QMAKE_EXTRA_TARGETS += check
