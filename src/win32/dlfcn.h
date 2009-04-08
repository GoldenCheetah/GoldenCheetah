// VERY simplistic implementation of dlfcn.h for win32 since it isn't a standard header on windows

#ifndef DLFCN_H
#define DLFCN_H

#define RTLD_NOW 2


void *dlopen (const char *file, int /*mode*/)
{
    return LoadLibraryA(file);
}

int dlclose (void *handle)
{
    return FreeLibrary((HINSTANCE)handle)==0 ? 0 : -1;
}

void *dlsym (void *handle, const char *name)
{
    return (void *)GetProcAddress((HINSTANCE)handle, name);
}

#endif // DLFCN_H
