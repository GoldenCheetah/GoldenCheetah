%Module goldencheetah

%MappedType QString /AllowNone,TypeHint="str",TypeHintValue="''"/
{
%TypeHeaderCode
#include <qstring.h>
%End

%ConvertToTypeCode
//*sipCppPtr = new QString(qpycore_PyObject_AsQString(sipPy));
//return sipGetState(sipTransferObj);
return 0;
%End
%ConvertFromTypeCode
    return PyUnicode_FromString(sipCpp->toLatin1().data());
%End
};

class Bindings {

%TypeHeaderCode
#include "Bindings.h"
%End

public:
    long threadid() const;
    QString athlete() const;
    int build() const;
    QString version() const;
};

