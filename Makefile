# Ensure consistent UTF-8 environment
export LANG := C.UTF-8
export LC_ALL := C.UTF-8

# Version information
VERSION        = WD-11.01
FULL_VERSION   = WD-251101
TARGET         ?= watchdogs       # Output binary name
TARGET_NAME    = Watchdogs

# Toolchain selection (user may override CC/STRIP via environment)
CC             ?= clang
STRIP          ?= llvm-strip
WINDRES        ?= windres         # Windows resource compiler

# Basic compile & link flags
CFLAGS   = -O2 -pipe -fdata-sections -ffunction-sections
LDFLAGS  = -lm -lcurl -lreadline -lncursesw -larchive

# Source files used in the build
SRCS = wd_extra.c wd_curl.c wd_unit.c wd_util.c wd_depends.c wd_hardware.c \
       wd_compiler.c wd_archive.c wd_package.c wd_server.c wd_crypto.c \
       include/tomlc/toml.c include/cJSON/cJSON.c

# Convert all .c sources -> .o (object files)
OBJS = $(SRCS:.c=.o)

# Resource files for Windows builds
RCFILE  = version.rc
RESFILE = version.res

# Mark non-file targets
.PHONY: init all clean linux termux windows compress strip debug termux-debug windows-debug

init:
	@echo "==> Detecting system environment..."
	@if [ -f "/.dockerenv" ]; then \
	    echo "==> Detected: Docker environment"; \
	    if ! command -v sudo >/dev/null 2>&1; then \
	    	echo "==> Can't found sudo.. installing..."; \
	    	if command -v apt >/dev/null 2>&1; then \
				apt update -y && apt install -y sudo; \
			elif command -v dnf >/dev/null 2>&1; then \
				dnf install -y sudo; \
			elif command -v yum >/dev/null 2>&1; then \
				yum install -y sudo; \
			elif command -v zypper >/dev/null 2>&1; then \
				zypper refresh && zypper install -y -t sudo; \
			elif command -v pacman >/dev/null 2>&1; then \
				pacman -Sy --noconfirm && pacman -S --needed --noconfirm sudo; \
			fi; \
	    fi; \
	fi; \
	\
	UNAME_S=$$(uname -s); \
	\
	if echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "==> Detected: MSYS2 MinGW UCRT64 environment"; \
		pacman -Sy --noconfirm && \
		pacman -S --needed --noconfirm \
			curl clang base-devel \
			mingw-w64-ucrt-x86_64-gcc \
			mingw-w64-ucrt-x86_64-lld \
			mingw-w64-ucrt-x86_64-libc++ \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-ncurses \
			mingw-w64-ucrt-x86_64-libarchive \
			mingw-w64-ucrt-x86_64-upx; \
	\
	elif echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
		echo "==> Detected: Termux environment"; \
		pkg update -y && pkg install -y unstable-repo coreutils procps clang curl libarchive libandroid-spawn ncurses readline; \
	\
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		echo "==> Native Linux"; \
		if command -v apt >/dev/null 2>&1; then \
			apt update -y && apt install -y build-essential curl procps clang lld make \
				libcurl4-openssl-dev libatomic1 libreadline-dev libarchive-dev \
				libncursesw5-dev libncurses5-dev zlib1g-dev; \
		elif command -v dnf >/dev/null 2>&1; then \
			dnf groupinstall -y "Development Tools" && \
			dnf install -y clang lld libatomic libcxx-devel ncurses-devel curl-devel \
				readline-devel libarchive-devel zlib-devel; \
		elif command -v yum >/dev/null 2>&1; then \
			yum groupinstall -y "Development Tools" && \
			yum install -y clang lld libcxx-devel ncurses-devel curl-devel \
				readline-devel libarchive-devel zlib-devel; \
		elif command -v zypper >/dev/null 2>&1; then \
			zypper refresh && zypper install -y -t pattern devel_basis && \
			zypper install -y curl clang lld libc++-devel ncurses-devel \
				libcurl-devel readline-devel libarchive-devel zlib-devel; \
		elif command -v pacman >/dev/null 2>&1; then \
			pacman -Sy --noconfirm && pacman -S --needed --noconfirm \
				libatomic base-devel clang lld libc++ ncurses readline curl libarchive zlib; \
		else \
			echo "==> Unsupported distribution"; \
			exit 1; \
		fi; \
	else \
		echo "==> Unknown environment"; \
		exit 1; \
	fi;

all: $(TARGET)
	@echo "==> Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)"
	@echo "==> Build complete: $(TARGET)"
	@$(MAKE) compress

$(TARGET): $(OBJS) $(RESFILE)
	@echo "==> Linking $(TARGET)"
	$(CC) $(CFLAGS) $(OBJS) $(RESFILE) -o $(TARGET) $(LDFLAGS)
	@$(MAKE) strip

%.o: %.c
	@echo "==> Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(RESFILE): $(RCFILE)
	@echo "==> Compiling resource file..."
	$(WINDRES) $(RCFILE) -O coff -o $(RESFILE)

# Strip
strip:
	@if [ -f "$(TARGET)" ]; then \
		echo "==> Stripping binary..."; \
		$(STRIP) --strip-all $(TARGET) || true; \
	else \
		echo "==> Nothing to strip"; \
	fi

# Compressing
compress:
	@if command -v upx >/dev/null 2>&1; then \
		echo "==> Compressing with UPX..."; \
		upx --best --lzma $(TARGET) || true; \
	else \
		echo "==> UPX not found, skipping compression."; \
	fi

# Cleaning
clean:
	rm -rf $(OBJS) $(RESFILE) version.res watchdogs watchdogs.tmux watchdogs.win watchdogs.debug watchdogs.debug.tmux watchdogs.debug.win

# Linux
linux:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building for GNU/Linux"
	@if [ -f "/usr/include/ncurses.h" ]; then \
		echo "==> ncurses found, building with _NCURSES flag"; \
		echo "==> Running: $(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -D_NCURSES -o $(TARGET) $(LDFLAGS)"; \
		$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -D_NCURSES -o $(TARGET) $(LDFLAGS); \
	else \
		echo "==> ncurses not found, building without _NCURSES flag"; \
		echo "==> Running: $(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -o $(TARGET) $(LDFLAGS)"; \
		$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -o $(TARGET) $(LDFLAGS); \
	fi;
	@echo "==> Linux build complete"

# Termux / Android
termux:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building for Termux (Android)"
	@if [ -f "/usr/include/ncurses.h" ] || [ -f "/data/data/com.termux/files/usr/include/ncurses" ]; then \
		echo "==> ncurses found, building with _NCURSES flag"; \
		echo "==> Running: $(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ -D_NCURSES -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -landroid-spawn -pie"; \
		$(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ -D_NCURSES -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -landroid-spawn -pie; \
	else \
		echo "==> ncurses not found, building without _NCURSES flag"; \
		echo "==> Running: $(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -landroid-spawn -pie"; \
		$(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -landroid-spawn -pie; \
	fi;
	@echo "==> Termux build complete"

# Windows (MSYS2 UCRT)
windows: $(RESFILE)
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building for Windows"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) $(RESFILE) -o watchdogs.win $(LDFLAGS) -liphlpapi -lshlwapi
	@echo "==> Windows build complete"

# Debug Build
debug:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building DEBUG version"
	@if [ -f "/usr/include/ncurses.h" ]; then \
		echo "==> ncurses found, building with _NCURSES flag and debug flags"; \
		echo "==> Running: $(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -D_NCURSES -Wall -o watchdogs.debug $(LDFLAGS)"; \
		$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -D_NCURSES -Wall -o watchdogs.debug $(LDFLAGS); \
	else \
		echo "==> ncurses not found, building without _NCURSES flag and debug flags"; \
		echo "==> Running: $(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug $(LDFLAGS)"; \
		$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug $(LDFLAGS); \
	fi;
	@echo "==> Debug build complete"

# Termux Debug Build
termux-debug:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building DEBUG Termux version"
	@if [ -f "/usr/include/ncurses.h" ] || [ -f "/data/data/com.termux/files/usr/include/ncurses" ]; then \
		echo "==> ncurses found, building with _NCURSES flag and debug flags"; \
		echo "==> Running: $(CC) -g -O0 -Wall -fno-omit-frame-pointer -fno-inline -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ -D_NCURSES $(SRCS) -o watchdogs.debug.tmux $(LDFLAGS) -landroid-spawn"; \
		$(CC) -g -O0 -Wall -fno-omit-frame-pointer -fno-inline -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ -D_NCURSES $(SRCS) -o watchdogs.debug.tmux $(LDFLAGS) -landroid-spawn; \
	else \
		echo "==> ncurses not found, building without _NCURSES flag and debug flags"; \
		echo "==> Running: $(CC) -g -O0 -Wall -fno-omit-frame-pointer -fno-inline -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ $(SRCS) -o watchdogs.debug.tmux $(LDFLAGS) -landroid-spawn"; \
		$(CC) -g -O0 -Wall -fno-omit-frame-pointer -fno-inline -I/data/data/com.termux/files/usr/include -I$$PREFIX/include -I$$PREFIX/lib -I$$PREFIX/bin -D__ANDROID__ $(SRCS) -o watchdogs.debug.tmux $(LDFLAGS) -landroid-spawn; \
	fi;
	@echo "==> Termux debug build complete"

# Windows Debug Build
windows-debug: $(RESFILE)
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building DEBUG Windows version"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) $(RESFILE) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug.win $(LDFLAGS) -liphlpapi -lshlwapi
	@echo "==> Debug build complete"
