
ifeq ($(QT4_MODE),1)
RAW_PLATFORM_DIRS+=qt4
endif

ifeq ($(WX_MODE),1)
RAW_PLATFORM_DIRS+=wx
endif


# mingw32 is a subset of win32
ifeq ($(TARGET_PLATFORM_MINGW32),1)
PROJECT_ARCHITECTURE?=i386
TARGET_PLATFORM_NAME=win32-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=MINGW32 WIN32
EXE=.exe
RAW_PLATFORM_DIRS+=win32 mingw32
TARGET_PLATFORM_WIN32=1
endif

ifeq ($(TARGET_PLATFORM_MINGW32)$(RELEASE),11)
LDFLAGS+=-s
endif

# cygwin is really a subset of posix
ifeq ($(TARGET_PLATFORM_CYGWIN),1)
PROJECT_ARCHITECTURE?=i386
TARGET_PLATFORM_NAME=cygwin-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=CYGWIN POSIX
EXE=.exe
RAW_PLATFORM_DIRS+=cygwin posix
endif

# plain posix is just posix
ifeq ($(TARGET_PLATFORM_POSIX),1)
PROJECT_ARCHITECTURE?=unknown
TARGET_PLATFORM_NAME=posix-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX
RAW_PLATFORM_DIRS+=posix
endif

# linux is a subset of posix
ifeq ($(TARGET_PLATFORM_LINUX),1)
PROJECT_ARCHITECTURE_SHELL:=$(shell uname -m)
PROJECT_ARCHITECTURE?=$(PROJECT_ARCHITECTURE_SHELL)
TARGET_PLATFORM_NAME=linux-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX
RAW_PLATFORM_DIRS+=posix linux
TARGET_PLATFORM_POSIX=1
endif


# linux-i386 is a subset of linux, posix
ifeq ($(TARGET_PLATFORM_LINUX_I386),1)
PROJECT_ARCHITECTURE?=i386
TARGET_PLATFORM_NAME=linux-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_I386
RAW_PLATFORM_DIRS+=posix linux linux-i386
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif

# linux-x86_64 is a subset of linux, posix, linux-i386
ifeq ($(TARGET_PLATFORM_LINUX_X86_64),1)
PROJECT_ARCHITECTURE?=x86_64
TARGET_PLATFORM_NAME=linux-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_I386  LINUX_X86_64
RAW_PLATFORM_DIRS+=posix linux linux-i386 linux-x86_64
COMPILE_FLAGS+=-m64
LINK_FLAGS+=-m64
TARGET_PLATFORM_LINUX_I386?=1
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif

# linux-ppc is a subset of linux, posix
ifeq ($(TARGET_PLATFORM_LINUX_PPC),1)
PROJECT_ARCHITECTURE?=ppc
TARGET_PLATFORM_NAME=linux-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_PPC
RAW_PLATFORM_DIRS+=posix linux linux-ppc
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif


# linux-ppc64 is a subset of linux, linux-ppc posix
ifeq ($(TARGET_PLATFORM_LINUX_PPC64),1)
PROJECT_ARCHITECTURE?=ppc64
TARGET_PLATFORM_NAME=linux-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_PPC LINUX_PPC64
RAW_PLATFORM_DIRS+=posix linux linux-ppc linux-ppc64
COMPILE_FLAGS+=-m64
LINK_FLAGS+=-m64
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_LINUX_PPC?=1
TARGET_PLATFORM_POSIX=1
endif

# xenomai is a subset of posix, linux
ifeq ($(TARGET_PLATFORM_XENOMAI),1)
PROJECT_ARCHITECTURE_SHELL:=$(shell uname -m)
PROJECT_ARCHITECTURE?=$(PROJECT_ARCHITECTURE_SHELL)
TARGET_PLATFORM_NAME=xenomai-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX XENOMAI
RAW_PLATFORM_DIRS+=posix linux xenomai
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif


# xenomai-i386 is a subset of linux, posix
ifeq ($(TARGET_PLATFORM_XENOMAI_I386),1)
PROJECT_ARCHITECTURE?=i386
TARGET_PLATFORM_NAME=linux-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_I386 XENOMAI XENOMAI_I386
RAW_PLATFORM_DIRS+=posix linux linux-i386 xenomai xenomai-i386
TARGET_PLATFORM_XENOMAI?=1
TARGET_PLATFORM_LINUX_I386?=1
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif

# xenomai-x86_64 is a subset of linux, posix, linux-i386, xenomai xenomai-i386
ifeq ($(TARGET_PLATFORM_XENOMAI_X86_64),1)
PROJECT_ARCHITECTURE?=x86_64
TARGET_PLATFORM_NAME=xenomai-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_X86_64 XENOMAI XENOMAI_I386 XENOMAI_X86_64
RAW_PLATFORM_DIRS+=posix linux linux-x86_64 xenomai xenomai-i386 xenomai-x86_64
COMPILE_FLAGS+=-m64
LINK_FLAGS+=-m64
TARGET_PLATFORM_XENOMAI_I386?=1
TARGET_PLATFORM_XENOMAI?=1
TARGET_PLATFORM_LINUX_X86_64?=1
TARGET_PLATFORM_LINUX_I386?=1
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif

# xenomai-ppc is a subset of linux, posix, xenomai
ifeq ($(TARGET_PLATFORM_XENOMAI_PPC),1)
PROJECT_ARCHITECTURE?=ppc
TARGET_PLATFORM_NAME=xenomai-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_PPC XENOMAI XENOMAI_PPC
RAW_PLATFORM_DIRS+=posix linux linux-ppc xenomai xenomai-ppc
TARGET_PLATFORM_XENOMAI_PPC?=1
TARGET_PLATFORM_XENOMAI?=1
TARGET_PLATFORM_LINUX_PPC?=1
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif

# xenomai-ppc64 is a subset of linux, linux-ppc posix xenomai xenomai_ppc
ifeq ($(TARGET_PLATFORM_XENOMAI_PPC64),1)
PROJECT_ARCHITECTURE?=ppc64
TARGET_PLATFORM_NAME=xenomai-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_PPC LINUX_PPC64 XENOMAI XENOMAI_PPC XENOMAI_PPC64
RAW_PLATFORM_DIRS+=posix linux linux-ppc linux-ppc64 xenomai xenomai-ppc xenomai-ppc64
COMPILE_FLAGS+=-m64
LINK_FLAGS+=-m64
TARGET_PLATFORM_XENOMAI_PPC?=1
TARGET_PLATFORM_XENOMAI?=1
TARGET_PLATFORM_LINUX_PPC64?=1
TARGET_PLATFORM_LINUX_PPC?=1
TARGET_PLATFORM_LINUX?=1
TARGET_PLATFORM_POSIX=1
endif


# bsd is a subset of posix
ifeq ($(TARGET_PLATFORM_BSD),1)
PROJECT_ARCHITECTURE?=unknown
TARGET_PLATFORM_NAME=bsd-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX BSD
RAW_PLATFORM_DIRS+=bsd posix
TARGET_PLATFORM_POSIX=1
endif

# freebsd is a subset of posix and bsd
ifeq ($(TARGET_PLATFORM_FREEBSD),1)
PROJECT_ARCHITECTURE?=unknown
TARGET_PLATFORM_NAME=freebsd-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX BSD FREEBSD
RAW_PLATFORM_DIRS+=bsd posix freebsd
TARGET_PLATFORM_POSIX=1
endif

# openbsd is a subset of posix and bsd
ifeq ($(TARGET_PLATFORM_OPENBSD),1)
PROJECT_ARCHITECTURE?=unknown
TARGET_PLATFORM_NAME=openbsd-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX BSD OPENBSD
RAW_PLATFORM_DIRS+=bsd posix openbsd
endif

# netbsd is a subset of posix and bsd
ifeq ($(TARGET_PLATFORM_NETBSD),1)
PROJECT_ARCHITECTURE?=unknown
TARGET_PLATFORM_NAME=netbsd-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX BSD NETBSD
RAW_PLATFORM_DIRS+=bsd posix netbsd
TARGET_PLATFORM_POSIX=1
endif

# solaris is a subset of posix 
ifeq ($(TARGET_PLATFORM_SOLARIS),1)
PROJECT_ARCHITECTURE?=unknown
TARGET_PLATFORM_NAME=solaris-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX SOLARIS
RAW_PLATFORM_DIRS+=solaris posix 
TARGET_PLATFORM_POSIX=1
endif

# macosx is a subset of posix
ifeq ($(TARGET_PLATFORM_MACOSX),1)
PROJECT_ARCHITECTURE_SHELL:=$(shell uname -p)
PROJECT_ARCHITECTURE?=$(PROJECT_ARCHITECTURE_SHELL)
TARGET_PLATFORM_NAME:=macosx-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX MACOSX
MACOSX_DEPLOYMENT_TARGET?=10.4
RAW_PLATFORM_DIRS+=posix macosx
DISASM=otool
DISASM_FLAGS=-t -v -V
COMPILE_FLAGS+=
LINK_FLAGS+=-mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TARGET_PLATFORM_POSIX=1
endif

# macosx_ppc is a subset of macosx and posix
ifeq ($(TARGET_PLATFORM_MACOSX_PPC),1)
PROJECT_ARCHITECTURE?=ppc
TARGET_PLATFORM_NAME=macosx-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX MACOSX MACOSX_PPC
MACOSX_DEPLOYMENT_TARGET?=10.4
RAW_PLATFORM_DIRS+=posix macosx macosx-ppc
DISASM=otool
DISASM_FLAGS=-t -v -V
COMPILE_FLAGS+=
LINK_FLAGS+=-mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TARGET_PLATFORM_MACOSX=1
TARGET_PLATFORM_POSIX=1
endif

# macosx_ppc64 is a subset of macosx and posix
ifeq ($(TARGET_PLATFORM_MACOSX_PPC64),1)
PROJECT_ARCHITECTURE?=ppc64
TARGET_PLATFORM_NAME=macosx-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX MACOSX MACOSX_PPC MACOSX_PPC64
MACOSX_DEPLOYMENT_TARGET?=10.4
RAW_PLATFORM_DIRS+=posix macosx macosx-ppc macosx-ppc64
DISASM=otool
DISASM_FLAGS=-t -v -V
COMPILE_FLAGS+=
LINK_FLAGS+=-mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TARGET_PLATFORM_MACOSX_PPC=1
TARGET_PLATFORM_POSIX=1
endif

# macosx_i386 is a subset of macosx and posix
ifeq ($(TARGET_PLATFORM_MACOSX_I386),1)
PROJECT_ARCHITECTURE?=i386
TARGET_PLATFORM_NAME=macosx-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX MACOSX MACOSX_I386
MACOSX_DEPLOYMENT_TARGET?=10.4
RAW_PLATFORM_DIRS+=posix macosx macosx-i386
DISASM=otool
DISASM_FLAGS=-t -v -V
COMPILE_FLAGS+=
LINK_FLAGS+=-mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TARGET_PLATFORM_MACOSX=1
TARGET_PLATFORM_POSIX=1
endif

# macosx_x86_64 is a subset of macosx and posix
ifeq ($(TARGET_PLATFORM_MACOSX_X86_64),1)
PROJECT_ARCHITECTURE?=x86_64
TARGET_PLATFORM_NAME=macosx-$(PROJECT_ARCHITECTURE)
SUFFIXES_TARGET_PLATFORM=POSIX MACOSX MACOSX_X86_64
MACOSX_DEPLOYMENT_TARGET?=10.4
RAW_PLATFORM_DIRS+=posix macosx macosx-x86_64
DISASM=otool
DISASM_FLAGS=-t -v -V
COMPILE_FLAGS+=
LINK_FLAGS+=-mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TARGET_PLATFORM_MACOSX=1
TARGET_PLATFORM_MACOSX_I386=1
TARGET_PLATFORM_POSIX=1
endif


# macosx_universal is a subset of macosx and posix and uses mac libtool to generate fat binaries.
MACOSX_DEPLOYMENT_TARGET?=10.4
TARGET_MACOSX_SDK?=/Developer/SDKs/MacOSX$(MACOSX_DEPLOYMENT_TARGET)u.sdk
ifeq ($(TARGET_PLATFORM_MACOSX_UNIVERSAL),1)
TARGET_PLATFORM_NAME=macosx-universal
MACOSX_UNIVERSAL_ARCHS?=i386 ppc
MACOSX_UNIVERSAL_ARCHS_PARAMS=$(foreach a,$(MACOSX_UNIVERSAL_ARCHS),-arch $(a))
SUFFIXES_TARGET_PLATFORM=POSIX MACOSX MACOSX_UNIVERSAL
RAW_PLATFORM_DIRS+=posix macosx macosx-ppc macosx-i386
COMPILE_FLAGS+=$(MACOSX_UNIVERSAL_ARCHS_PARAMS) -isysroot $(TARGET_MACOSX_SDK) 
PREPROCESS_FLAGS+=-isysroot $(TARGET_MACOSX_SDK) 
INCLUDES+=$(TARGET_MACOSX_SDK)/usr/include
LINK_FLAGS+=$(MACOSX_UNIVERSAL_ARCHS_PARAMS) -isysroot $(TARGET_MACOSX_SDK) -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TARGET_USE_AR=0
TARGET_USE_MACOSX_LIBTOOL=1
MACOSX_LIBTOOL=libtool
MACOSX_LIBTOOLFLAGS?=-static
DISASM=otool
DISASM_FLAGS=-t -v -V
PROJECT_ARCHITECTURE?=$(MACOSX_UNIVERSAL_ARCHS)
TARGET_PLATFORM_MACOSX=1
endif

# cell_spu is not a posix platform
ifeq ($(TARGET_PLATFORM_CELL_SPU),1)
TARGET_PLATFORM_NAME=linux-cell_spu
SUFFIXES_TARGET_PLATFORM=CELL_SPU
RAW_PLATFORM_DIRS+=cell_spu
PROJECT_ARCHITECTURE?=cell_spu
endif

# linux_cell_ppu is a subset of linux-ppc, linux, posix
ifeq ($(TARGET_PLATFORM_LINUX_CELL_PPU),1)
TARGET_PLATFORM_NAME=linux-cell_ppu
SUFFIXES_TARGET_PLATFORM=POSIX LINUX LINUX_PPC LINUX_CELL_PPU CELL_PPU
RAW_PLATFORM_DIRS+=posix linux cell_ppu
PROJECT_ARCHITECTURE?=cell_ppu
TARGET_PLATFORM_POSIX=1
TARGET_PLATFORM_LINUX=1
endif

# default to objdump for disassembly
DISASM_FLAGS?=-d -S
DISASM?=$(OBJDUMP)

# if EXE suffix is not set then it ought to be blank.
EXE?=

# If we are to use normal gnu 'ar' to manipulate static libraries for the target platform,
# TARGET_USE_AR is to be set to 1. Defaults to 1.
TARGET_USE_AR?=1

# If we are to use mac os x libtool (tiger and beyond) to manipulate static libraries for the target
# platform, TARGET_USE_MACOSX_LIBTOOL is to be set to 1 instead. Defaults to 0.
TARGET_USE_MACOSX_LIBTOOL?=0

PLATFORM_DIRS=$(sort $(RAW_PLATFORM_DIRS))
