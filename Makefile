export LANG := C.UTF-8
export LC_ALL := C.UTF-8

VERSION        = WG-12.23
FULL_VERSION   = WG-251223

TARGET         ?= watchdogs
OUTPUT         ?= $(TARGET)
TARGET_NAME    = Watchdogs

CC       ?= clang
STRIP    ?= strip

CFLAGS   = -Os -pipe
LDFLAGS  = -lm -lcurl -lreadline -larchive

ifeq ($(DEBUG_MODE),1)
	CFLAGS = -g -O0 -Wall -fdata-sections -ffunction-sections -fno-omit-frame-pointer -fno-inline
	STRIP  = true
endif

SRCS = source/extra.c source/debug.c source/curl.c source/units.c source/utils.c source/depend.c \
       source/compiler.c source/archive.c source/library.c source/runner.c source/crypto.c \
       include/tomlc/toml.c include/cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: init strip compress clean linux termux debug termux-debug windows-debug

init:
		@echo "==> Detecting environment..."
		@UNAME_S="$$(uname -s)"; \
		if echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
			echo "==> Using pacman (MSYS2)"; \
			pacman -Sy --noconfirm && \
			pacman -S --needed --noconfirm \
			  curl \
			  base-devel \
			  mingw-w64-ucrt-x86_64-clang \
			  mingw-w64-ucrt-x86_64-gcc \
			  mingw-w64-ucrt-x86_64-lld \
			  mingw-w64-ucrt-x86_64-libc++ \
			  mingw-w64-ucrt-x86_64-curl \
			  mingw-w64-ucrt-x86_64-readline \
			  mingw-w64-ucrt-x86_64-libarchive \
			  mingw-w64-ucrt-x86_64-upx; \
		elif echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
			echo "==> Using apt (Termux)"; \
			apt -o Acquire::Queue-Mode=access -o Acquire::Retries=3 update -y && \
			DEBIAN_FRONTEND=noninteractive \
			apt -o Dpkg::Use-Pty=0 install -y --no-install-recommends \
				x11-repo unstable-repo \
				coreutils binutils procps clang curl \
				libarchive libandroid-spawn readline upx; \
		elif echo "$$UNAME_S" | grep -qi "Linux"; then \
			if command -v apt >/dev/null 2>&1; then \
				echo "==> Using apt (Debian/Ubuntu)"; \
				dpkg --add-architecture i386 2>/dev/null || true; \
				apt -o Acquire::Queue-Mode=access -o Acquire::Retries=3 update -y && \
				DEBIAN_FRONTEND=noninteractive \
				apt -o Dpkg::Use-Pty=0 install -y --no-install-recommends \
					build-essential curl procps clang lld make binutils \
					libcurl4-openssl-dev libatomic1 libreadline-dev libarchive-dev \
					zlib1g-dev upx-ucl upx \
					libc6:i386 libstdc++6:i386 libcurl4:i386; \
			elif command -v dnf >/dev/null 2>&1; then \
				echo "==> Using dnf (Fedora/RHEL)"; \
				dnf -y groupinstall "Development Tools" && \
				dnf -y install \
					clang lld libatomic libcxx-devel curl-devel \
					readline-devel libarchive-devel \
					zlib-devel binutils upx \
					libstdc++-devel.i686 curl-devel.i686; \
			elif command -v yum >/dev/null 2>&1; then \
				echo "==> Using yum (Legacy RHEL)"; \
				yum -y groupinstall "Development Tools" && \
				yum -y install \
					clang lld libcxx-devel curl-devel \
					readline-devel libarchive-devel \
					zlib-devel binutils upx \
					libstdc++-devel.i686 curl-devel.i686; \
			elif command -v zypper >/dev/null 2>&1; then \
				echo "==> Using zypper (openSUSE)"; \
				zypper --non-interactive refresh && \
				zypper --non-interactive install -y -t pattern devel_basis && \
				zypper --non-interactive install -y \
					curl clang lld libc++-devel \
					libcurl-devel readline-devel libarchive-devel \
					zlib-devel binutils upx \
					libstdc++6-devel-32bit libcurl4-devel-32bit; \
			elif command -v pacman >/dev/null 2>&1; then \
				echo "==> Using pacman (Arch)"; \
				pacman -Sy --noconfirm && \
				pacman -S --needed --noconfirm \
					libatomic base-devel clang lld libc++ readline \
					curl libarchive zlib binutils upx \
					lib32-gcc-libs; \
			else \
				echo "Unsupported Linux distribution"; \
				exit 1; \
			fi; \
		fi

$(OUTPUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTPUT) $(LDFLAGS)
	@$(MAKE) strip OUTPUT=$(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

strip:
	@if [ -f "$(OUTPUT)" ]; then $(STRIP) --strip-all $(OUTPUT) || true; fi

compress:
	@if [ -f "$(OUTPUT)" ] && command -v upx >/dev/null 2>&1; then upx -v --best --lzma "$(OUTPUT)"; fi

linux: OUTPUT = watchdogs
linux:
	$(CC) $(CFLAGS) -D__LINUX__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS); \
	$(MAKE) strip OUTPUT=$(OUTPUT); \
	$(MAKE) compress OUTPUT=$(OUTPUT)

termux-fast: OUTPUT = watchdogs.tmux
termux-fast:
	$(CC) $(CFLAGS_FAST) -D__ANDROID__ -D__W_VERSION__=\"$(FULL_VERSION)\" -fPIE $(SRCS) -o $(OUTPUT) $(LDFLAGS) -landroid-spawn -pie

termux: OUTPUT = watchdogs.tmux
termux:
	$(CC) $(CFLAGS) -D__ANDROID__ -D__W_VERSION__=\"$(FULL_VERSION)\" -fPIE $(SRCS) -o $(OUTPUT) $(LDFLAGS) -landroid-spawn -pie
	@$(MAKE) strip OUTPUT=$(OUTPUT)
	@$(MAKE) compress OUTPUT=$(OUTPUT)

windows: OUTPUT = watchdogs.win
windows:
	$(CC) $(CFLAGS) $(SRCS) -D__WINDOWS32__ -D__W_VERSION__=\"$(FULL_VERSION)\" -o $(OUTPUT) $(LDFLAGS)
	@$(MAKE) strip OUTPUT=$(OUTPUT)
	@$(MAKE) compress OUTPUT=$(OUTPUT)

debug: DEBUG_MODE=1
debug: OUTPUT = watchdogs.debug
debug:
	$(CC) $(CFLAGS) -g -D_DBG_PRINT -D__LINUX__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS)

termux-debug: DEBUG_MODE=1
termux-debug: OUTPUT = watchdogs.debug.tmux
termux-debug:
	$(CC) $(CFLAGS) -g -D_DBG_PRINT -D__ANDROID__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -landroid-spawn -o $(OUTPUT) $(LDFLAGS)

windows-debug: DEBUG_MODE=1
windows-debug: OUTPUT = watchdogs.debug.win
windows-debug:
	$(CC) $(CFLAGS) -g -D_DBG_PRINT -D__WINDOWS32__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(OUTPUT) watchdogs watchdogs.win watchdogs.tmux \
	       watchdogs.debug watchdogs.debug.tmux watchdogs.debug.win
