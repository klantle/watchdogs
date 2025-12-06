# Standard UTF-8 environment

export LANG := C.UTF-8
export LC_ALL := C.UTF-8


# Version information

VERSION        = WG-12.06
FULL_VERSION   = WG-251206


# Default output names

TARGET         ?= watchdogs
OUTPUT         ?= $(TARGET)
TARGET_NAME    = Watchdogs


# Toolchain configuration

CC       ?= clang
STRIP    ?= strip
WINDRES  ?= windres


# Default optimization flags (for release mode)

CFLAGS   = -Os -pipe -fdata-sections -ffunction-sections
LDFLAGS  = -lm -lcurl -lreadline -larchive


# Debug-mode override
#
# When DEBUG_MODE=1:
# - All release optimizations are disabled
# - Full debug symbols enabled
# - Frame pointers kept intact (better backtrace)
# - Inline disabled for precise debugging
# - STRIP is replaced with "true" so it does nothing

ifeq ($(DEBUG_MODE),1)
	CFLAGS = -g -O0 -Wall -fno-omit-frame-pointer -fno-inline
	STRIP  = true
endif


# Source & object files

SRCS = source/extra.c source/debug.c source/curl.c source/units.c source/utils.c source/depend.c source/kernel.c \
       source/compiler.c source/archive.c source/package.c source/runner.c source/crypto.c \
       include/tomlc/toml.c include/cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

# Windows resource files
RCFILE  = VERSION.rc
RESFILE = VERSION.res

.PHONY: init clean linux termux windows strip compress debug termux-debug windows-debug


# Initialization routine for automatic toolchain setup
init:
	@echo "==> Detecting environment..."
	UNAME_S=$$(uname -s); \
	\
	if echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "Detected MSYS2 MinGW UCRT64"; \
		pacman -Sy --noconfirm && \
		pacman -S --needed --noconfirm \
			curl clang base-devel \
			mingw-w64-ucrt-x86_64-gcc \
			mingw-w64-ucrt-x86_64-lld \
			mingw-w64-ucrt-x86_64-libc++ \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-libarchive \
			mingw-w64-ucrt-x86_64-upx; \
	\
	elif echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
		echo "Detected Termux"; \
		pkg update -y && pkg install -y \
			x11-repo unstable-repo coreutils binutils procps clang curl \
			libarchive libandroid-spawn readline upx; \
	\
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		echo "Detected Linux"; \
		if command -v apt >/dev/null 2>&1; then \
			apt update -y && apt install -y \
				build-essential curl procps clang lld make binutils \
				libcurl4-openssl-dev libatomic1 libreadline-dev libarchive-dev \
				zlib1g-dev upx-ucl upx; \
		elif command -v dnf >/dev/null 2>&1; then \
			dnf groupinstall -y "Development Tools" && \
			dnf install -y \
				clang lld libatomic libcxx-devel curl-devel \
				readline-devel libarchive-devel zlib-devel binutils upx; \
		elif command -v yum >/dev/null 2>&1; then \
			yum groupinstall -y "Development Tools" && \
			yum install -y \
				clang lld libcxx-devel curl-devel \
				readline-devel libarchive-devel zlib-devel binutils upx; \
		elif command -v zypper >/dev/null 2>&1; then \
			zypper refresh && zypper install -y -t pattern devel_basis && \
			zypper install -y \
				curl clang lld libc++-devel \
				libcurl-devel readline-devel libarchive-devel zlib-devel \
				binutils upx; \
		elif command -v pacman >/dev/null 2>&1; then \
			pacman -Sy --noconfirm && pacman -S --needed --noconfirm \
				libatomic base-devel clang lld libc++ readline \
				curl libarchive zlib binutils upx; \
		else \
			echo "Unsupported distribution"; \
			exit 1; \
		fi; \
	else \
		echo "Unsupported environment"; \
		exit 1; \
	fi;


# Generic build pipeline

$(OUTPUT): $(OBJS) $(RESFILE)
	@echo "Linking $(OUTPUT)"
	$(CC) $(CFLAGS) $(OBJS) $(RESFILE) -o $(OUTPUT) $(LDFLAGS)
	@$(MAKE) strip OUTPUT=$(OUTPUT)

%.o: %.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(RESFILE): $(RCFILE)
	@echo "Compiling resource file"
	$(WINDRES) $(RCFILE) -O coff -o $(RESFILE)


# Strip and compression routines

strip:
	@if [ -f "$(OUTPUT)" ]; then \
		echo "Stripping $(OUTPUT)..."; \
		$(STRIP) --strip-all $(OUTPUT) || true; \
	else \
		echo "No file to strip: $(OUTPUT)"; \
	fi

compress:
	@if [ -f "$(OUTPUT)" ] && command -v upx >/dev/null 2>&1; then \
		echo "Compressing $(OUTPUT) with UPX"; \
		upx -v --best --lzma "$(OUTPUT)"; \
	else \
		echo "Skipping compression"; \
	fi


# Linux build (release)

linux: OUTPUT = watchdogs
linux:
	@echo "Building for Linux"
	$(CC) $(CFLAGS) -D__LINUX__ -I/usr/include/ $(SRCS) -o $(OUTPUT) $(LDFLAGS); \
	@$(MAKE) strip OUTPUT=$(OUTPUT)
	@$(MAKE) compress OUTPUT=$(OUTPUT)


# Termux build (release)

termux: OUTPUT = watchdogs.tmux
termux:
	@echo "Building for Termux"
	$(CC) $(CFLAGS) -I$$PREFIX/include -I$$PREFIX/lib -D__ANDROID__ -fPIE $(SRCS) -o $(OUTPUT) $(LDFLAGS) -landroid-spawn -pie
	@$(MAKE) strip OUTPUT=$(OUTPUT)
	@$(MAKE) compress OUTPUT=$(OUTPUT)


# Windows build (release)

windows: OUTPUT = watchdogs.win
windows: $(RESFILE)
	@echo "Building for Windows"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) $(RESFILE) -D__WINDOWS__ -o $(OUTPUT) $(LDFLAGS)
	@$(MAKE) strip OUTPUT=$(OUTPUT)
	@$(MAKE) compress OUTPUT=$(OUTPUT)


# Debug builds (strip disabled automatically)

debug: DEBUG_MODE=1
debug: OUTPUT = watchdogs.debug
debug:
	$(CC) $(CFLAGS) -D_DBG_PRINT -D__LINUX__ $(SRCS) -o $(OUTPUT) $(LDFLAGS)
	@echo "Debug build complete"

termux-debug: DEBUG_MODE=1
termux-debug: OUTPUT = watchdogs.debug.tmux
termux-debug:
	$(CC) $(CFLAGS) -D_DBG_PRINT -D__ANDROID__ $(SRCS) -landroid-spawn -o $(OUTPUT) $(LDFLAGS)
	@echo "Termux debug build complete"

windows-debug: DEBUG_MODE=1
windows-debug: OUTPUT = watchdogs.debug.win
windows-debug: $(RESFILE)
	$(CC) $(CFLAGS) -D_DBG_PRINT -D__WINDOWS__ $(SRCS) $(RESFILE) -o $(OUTPUT) $(LDFLAGS)
	@echo "Windows debug build complete"


# Clean build artifacts

clean:
	rm -rf $(OBJS) $(RESFILE) $(OUTPUT) watchdogs watchdogs.win watchdogs.tmux \
	       watchdogs.debug watchdogs.debug.tmux watchdogs.debug.win
