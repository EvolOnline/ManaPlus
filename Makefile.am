# Subdirectories to build
SUBDIRS = data docs po src

desktopdir = $(datarootdir)/applications

desktop_DATA = manaplus.desktop \
	     manaplustest.desktop

# Extra files to include
EXTRA_DIST = $(desktop_DATA) \
	     CMakeLists.txt \
	     po/CMakeLists.txt \
	     po/*.po \
	     CMake/Modules/FindENet.cmake \
	     CMake/Modules/FindLibIntl.cmake \
	     CMake/Modules/FindSDL_gfx.cmake \
	     manaplus.cbp \
	     manaplus.menu \
	     manaplus.spec \
	     AUTHORS \
	     ChangeLog \
	     INSTALL \
	     PKGBUILD \
	     README \
	     README.cmake

# Autopoint m4 stuff
ACLOCAL_AMFLAGS = -I m4
