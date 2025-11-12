UNAME_S := $(shell uname -s)

export TERM
MAKE_TERMOUT := 1

export LANG := C.UTF-8
export LC_ALL := C.UTF-8

VERSION  	   = WD-11.01
FULL_VERSION   = WD-251101
TARGET   	   ?= watchdogs
CC       	   ?= clang
STRIP          ?= llvm-strip
WINDRES        ?= windres

CFLAGS   = -pipe -fdata-sections -ffunction-sections
LDFLAGS  =  -lm -lcurl -lreadline -lncursesw -larchive

SRCS = wd_extra.c wd_curl.c wd_unit.c wd_util.c wd_depends.c wd_hardware.c \
	wd_compiler.c wd_archive.c wd_package.c wd_server.c wd_crypto.c \
       include/tomlc/toml.c include/cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)
RCFILE = version.rc
RESFILE = version.res

.PHONY: init all clean linux termux windows compress strip debug termux-debug windows-debug

init:
	@echo "==> Detecting system environment..."
	@UNAME_S=$$(uname -s); \
	if echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "==> Detected: MSYS2 MinGW UCRT64 environment"; \
		pacman -Sy --noconfirm && \
		pacman -S --needed --noconfirm \
			curl \
			clang \
			base-devel \
			mingw-w64-ucrt-x86_64-gcc \
			mingw-w64-ucrt-x86_64-lld \
			mingw-w64-ucrt-x86_64-libc++ \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-ncurses \
			mingw-w64-ucrt-x86_64-libarchive \
			mingw-w64-ucrt-x86_64-upx; \
	elif echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
		echo "==> Detected: Termux environment"; \
		pkg update -y && pkg install -y unstable-repo coreutils procps clang curl libarchive ncurses readline; \
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		echo "==> Detected: Native Linux environment"; \
		echo "==> Installing dependencies without sudo..."; \
		if command -v apt >/dev/null 2>&1; then \
			apt update -y && \
			apt install -y build-essential curl procps clang lld make \
				libcurl4-openssl-dev libncursesw5-dev libreadline-dev \
				libarchive-dev zlib1g-dev libonig-dev; \
		elif command -v dnf >/dev/null 2>&1; then \
			dnf groupinstall -y "Development Tools" && \
			dnf install -y clang lld libcxx-devel ncurses-devel \
				curl-devel readline-devel libarchive-devel zlib-devel; \
		elif command -v yum >/dev/null 2>&1; then \
			yum groupinstall -y "Development Tools" && \
			yum install -y clang lld libcxx-devel ncurses-devel \
				curl-devel readline-devel libarchive-devel zlib-devel; \
		elif command -v zypper >/dev/null 2>&1; then \
			zypper refresh && \
			zypper install -y -t pattern devel_basis && \
			zypper install -y curl clang lld libc++-devel ncurses-devel \
				libcurl-devel readline-devel libarchive-devel zlib-devel; \
		elif command -v pacman >/dev/null 2>&1; then \
			pacman -Sy --noconfirm && \
			pacman -S --needed --noconfirm base-devel clang lld libc++ ncurses \
				curl readline libarchive zlib; \
		else \
			echo "==> Cannot install dependencies: package manager not found"; \
			echo "==> Please install dependencies manually."; \
			exit 1; \
		fi; \
	else \
		echo "==> Unknown environment."; \
		exit 1; \
	fi

all: $(TARGET)
	@echo "==> Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)"
	@echo "==> Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)"
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

strip:
	@if [ -f "$(TARGET)" ]; then \
		echo "==> Stripping binary..."; \
		$(STRIP) --strip-all $(TARGET) || true; \
	else \
		echo "==> Nothing to strip"; \
	fi

compress:
	@if command -v upx >/dev/null 2>&1; then \
		echo "==> Compressing with UPX..."; \
		upx --best --lzma $(TARGET) || true; \
	else \
		echo "==> UPX not found, skipping compression."; \
	fi

clean:
	rm -rf $(OBJS) $(RESFILE) watchdogs watchdogs.tmux watchdogs.win watchdogs.debug watchdogs.debug.tmux watchdogs.debug.win
	@echo "==> Clean done."

linux:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)"
	$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "==> Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)"

termux:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building Termux target with clang..."
	$(CC) $(CFLAGS) -I/data/data/com.termux/files/usr/include -I$PREFIX/include -I$PREFIX/lib -I$PREFIX/bin -D__ANDROID__ -fPIE $(SRCS) -o watchdogs.tmux $(LDFLAGS) -pie
	@echo "==> Build complete: watchdogs.tmux Version $(VERSION) Full Version $(FULL_VERSION)"

windows: $(RESFILE)
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building Windows target (MinGW)..."
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) $(RESFILE) -o watchdogs.win $(LDFLAGS) -liphlpapi -lshlwapi
	@echo "==> Build complete: watchdogs.win Version $(VERSION) Full Version $(FULL_VERSION)"

debug:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)"
	$(CC) $(CFLAGS) -I/usr/include/ $(SRCS) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug $(LDFLAGS)
	@echo "==> Build complete: watchdogs.debug Version $(VERSION) Full Version $(FULL_VERSION)"

termux-debug:
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)"
	$(CC) -g -O0 -Wall -fno-omit-frame-pointer -fno-inline -I/data/data/com.termux/files/usr/include -I$PREFIX/include -I$PREFIX/lib -I$PREFIX/bin -D__ANDROID__ $(SRCS) -o watchdogs.debug.tmux $(LDFLAGS)
	@echo "==> Build complete: watchdogs.debug.tmux Version $(VERSION) Full Version $(FULL_VERSION)"

windows-debug: $(RESFILE)
	@echo "-> [LANG = $$LANG]"
	@echo "==> Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)"
	$(CC) $(CFLAGS) -I/ucrt64/include $(SRCS) $(RESFILE) -g -O0 -D_DBG_PRINT -Wall -o watchdogs.debug.win $(LDFLAGS) -liphlpapi -lshlwapi
	@echo "==> Debug build complete: ./watchdogs.debug.win"
