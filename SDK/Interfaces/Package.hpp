#ifndef PACKAGE_HPP
#define PACKAGE_HPP

class ApplicationCore;
using InitPackageFunc = void(ApplicationCore*);

extern "C" InitPackageFunc initPackage;

#endif
