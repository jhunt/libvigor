Name:           libvigor
Version:        1.2.7
Release:        1%{?dist}
Summary:        Missing Bits of C

Group:          System Environment/Libraries
License:        GPLv3+
URL:            https://github.com/filefrog/vigor
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  gcc
BuildRequires:  libtool
BuildRequires:  libctap
BuildRequires:  libctap-devel
BuildRequires:  zeromq-devel
BuildRequires:  libsodium-devel

%description
libvigor is a set of primitives for getting past the inherent shortcomings
of a beautifully simple language like C.  It provides robust list and hash
representations, and several other abstractions like daemonization, command
execution management, a 0MQ packet layer, time and timing functions and more.

%package devel
Summary:        Missing Bits of C - Development Files
Group:          Development/Libraries

%description devel
libvigor is a set of primitives for getting past the inherent shortcomings
of a beautifully simple language like C.  It provides robust list and hash
representations, and several other abstractions like daemonization, command
execution management, a 0MQ packet layer, time and timing functions and more.

This package contains the header files for developing code against libvigor.


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT/usr/bin/fuzz-config


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_libdir}/libvigor.so
%{_libdir}/libvigor.so.1
%{_libdir}/libvigor.so.1.0.0

%files devel
%defattr(-,root,root,-)
%{_includedir}/vigor.h
%{_libdir}/libvigor.a
%{_libdir}/libvigor.la


%changelog
* Wed Jun 10 2015 James Hunt <james@niftylogic.com> 1.2.7-1
- Upstream release

* Tue May 19 2015 James Hunt <james@niftylogic.com> 1.2.6-1
- Initial RPM package
