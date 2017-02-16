In order to build the device support on a Windows machine, Boost C++ library (http://www.boost.org/) is needed. Extract it to an arbitrary location and provide path in the configure/CONFIG_SITE.local (e.g. BOOST = C:\boost_1_63_0).

Additionally, the testTop/ClientApp requires getopt in order to function. Windows version of getopt can be obtained from https://github.com/alex85k/wingetopt. Then add getopt src folder path to testTop/configure/CONFIG_SITE.local (e.g. GETOPT = C:\ ...).

The path to SDK dlls (uastack.dll, libxml2.dll ...) needs to be in your environment PATH variable for the applications to function.
