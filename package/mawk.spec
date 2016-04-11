Summary: mawk - pattern scanning and text processing language
%define AppProgram mawk
%define AppVersion 1.3.5
%define AppRelease 20150503
# $MawkId: mawk.spec,v 1.50 2015/05/03 22:04:28 tom Exp $
Name: %{AppProgram}
Version: %{AppVersion}
Release: %{AppRelease}
License: GPLv2
Group: Applications/Development
URL: ftp://invisible-island.net/%{AppProgram}
Source0: %{AppProgram}-%{AppVersion}-%{AppRelease}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
mawk is an interpreter for the AWK Programming Language.  The AWK language is
useful for manipulation of data files, text retrieval and processing, and for
prototyping and experimenting with algorithms.

%prep

%define debug_package %{nil}
%setup -q -n %{AppProgram}-%{AppVersion}-%{AppRelease}

%build

INSTALL_PROGRAM='${INSTALL}' \
	./configure \
		--target %{_target_platform} \
		--prefix=%{_prefix} \
		--bindir=%{_bindir} \
		--libdir=%{_libdir} \
		--mandir=%{_mandir}

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install                    DESTDIR=$RPM_BUILD_ROOT

strip $RPM_BUILD_ROOT%{_bindir}/%{AppProgram}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_prefix}/bin/%{AppProgram}
%{_mandir}/man1/%{AppProgram}.*

%changelog
# each patch should add its ChangeLog entries here

* Mon Apr 11 2016 jlp765
- update version to 1.3.5

* Sat Oct 27 2012 Thomas Dickey
- cancel any debug-rpm

* Sun Jun 20 2010 Thomas Dickey
- initial version