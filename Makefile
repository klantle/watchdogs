VERSION        = DOG-26.01
FULL_VERSION   = DOG-260101
TARGET        ?= watchdogs
OUTPUT        ?= $(TARGET)
TARGET_NAME    = Watchdogs
CC            ?= clang
CFLAGS         = -O2 -pipe
LDFLAGS        = -lm -lcurl -lreadline -larchive

ifeq ($(DEBUG_MODE),1)
CFLAGS = -std=c23 -ggdb3 -Og \
  -Wall -Wextra -Wpedantic -Werror \
  -Wconversion -Wsign-conversion -Wfloat-conversion \
  -Wshadow -Wundef \
  -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
  -Wnull-dereference -Wuninitialized -Wconditional-uninitialized \
  -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls \
  -fwrapv -fno-strict-aliasing \
  -fsanitize=address,undefined \
  -fno-sanitize-recover=all \
  -fdata-sections -ffunction-sections \
  -DDEBUG

LDFLAGS = -fsanitize=address,undefined -rdynamic
endif

SRCS = \
	source/extra.c \
	source/debug.c \
	source/curl.c \
	source/units.c \
	source/utils.c \
	source/replicate.c \
	source/cause.c \
	source/compiler.c \
	source/archive.c \
	source/library.c \
	source/endpoint.c \
	source/crypto.c \
	include/tomlc/toml.c \
	include/cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: init clean linux termux debug termux-debug windows-debug

init:
	@echo "==> Detecting environment..."
	@UNAME_S="$$(uname -s)"; \
	if echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "==> Using pacman (MSYS2)"; \
		pacman -Sy --noconfirm && \
		pacman -S --needed --noconfirm \
			curl \
			base-devel \
			mingw-w64-ucrt-x86_64-libc++ \
			mingw-w64-ucrt-x86_64-clang \
			mingw-w64-ucrt-x86_64-gcc \
			mingw-w64-ucrt-x86_64-lld \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-libarchive \
			procps-ng; \
	elif echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
		echo "==> Using apt (Termux)"; \
		apt -o Acquire::Queue-Mode=access -o Acquire::Retries=3 update -y && \
		DEBIAN_FRONTEND=noninteractive \
		apt -o Dpkg::Use-Pty=0 install -y --no-install-recommends \
			unstable-repo coreutils binutils procps clang curl \
			libarchive readline; \
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		if command -v apt >/dev/null 2>&1; then \
			echo "==> Using apt (Debian/Ubuntu)"; \
			dpkg --add-architecture i386 2>/dev/null || true; \
			apt -o Acquire::Queue-Mode=access -o Acquire::Retries=3 update -y && \
			DEBIAN_FRONTEND=noninteractive \
			apt -o Dpkg::Use-Pty=0 install -y --no-install-recommends \
				build-essential curl procps clang lld make binutils \
				libcurl4-openssl-dev libatomic1 libreadline-dev libarchive-dev \
				zlib1g-dev libc6:i386 libstdc++6:i386 libcurl4:i386; \
		elif command -v dnf >/dev/null 2>&1 || command -v dnf5 >/dev/null 2>&1; then \
			echo "==> Using dnf/dnf5 (Fedora/RHEL/AlmaLinux/Rocky Linux)"; \
			if [ -f /etc/almalinux-release ] || [ -f /etc/rocky-release ] || [ -f /etc/redhat-release ]; then \
				echo "==> Detected RHEL-based distribution (AlmaLinux/Rocky Linux/RHEL)"; \
				dnf -y install epel-release dnf-plugins-core 2>/dev/null || true; \
				dnf config-manager --set-enabled crb 2>/dev/null || true; \
			fi; \
			if command -v dnf5 >/dev/null 2>&1; then \
				DNF_CMD="dnf5"; \
				echo "==> Detected dnf5 (Fedora 39+)"; \
				$$DNF_CMD install -y 'dnf5-command(group)' 2>/dev/null || true; \
			else \
				DNF_CMD="dnf"; \
				echo "==> Detected dnf (Fedora <= 38 / RHEL-based)"; \
			fi; \
			$$DNF_CMD -y update; \
			if [ "$$DNF_CMD" = "dnf5" ]; then \
				echo "==> Installing Development Tools with dnf5"; \
				$$DNF_CMD -y install '@Development Tools' || \
				$$DNF_CMD -y install @development-tools; \
			else \
				echo "==> Installing Development Tools with dnf"; \
				$$DNF_CMD -y groupinstall 'Development Tools'; \
			fi; \
			echo "==> Installing additional dependencies"; \
			$$DNF_CMD -y install libcxx-devel 2>/dev/null || echo "==> libcxx-devel not available, skipping..."; \
			if $$DNF_CMD list 'curl-devel.i686' >/dev/null 2>&1; then \
				echo "==> 32-bit packages available, installing both architectures"; \
				$$DNF_CMD -y install \
					clang lld libatomic curl-devel \
					readline-devel libarchive-devel \
					zlib-devel binutils procps-ng file \
					glibc-devel.i686 libstdc++-devel.i686; \
				$$DNF_CMD -y install curl-devel.i686 2>/dev/null || echo "==> curl-devel.i686 not available, skipping..."; \
			else \
				echo "==> 32-bit packages not available, installing only 64-bit"; \
				$$DNF_CMD -y install \
					clang lld libatomic curl-devel \
					readline-devel libarchive-devel \
					zlib-devel binutils procps-ng file \
					glibc.i686 libstdc++.i686; \
			fi; \
			$$DNF_CMD -y install llvm-toolset 2>/dev/null || echo "==> llvm-toolset not available, skipping..."; \
		elif command -v zypper >/dev/null 2>&1; then \
			echo "==> Using zypper (openSUSE)"; \
			zypper --non-interactive refresh && \
			zypper --non-interactive install -y -t pattern devel_basis && \
			echo "==> Installing additional dependencies for openSUSE (64-bit only)"; \
			zypper --non-interactive install -y \
				curl \
				clang lld llvm \
				libc++-devel \
				libatomic1 \
				libcurl-devel \
				readline-devel \
				libarchive-devel \
				binutils \
				procps \
				libstdc++6-32bit; \
		elif command -v pacman >/dev/null 2>&1; then \
			echo "==> Using pacman (Arch)"; \
			pacman -Syu --noconfirm && \
			pacman -S --needed --noconfirm \
				base-devel \
				clang lld llvm libc++ \
				libatomic_ops \
				readline \
				curl \
				libarchive \
				zlib \
				binutils \
				procps-ng \
				lib32-gcc-libs; \
		fi; \
	fi

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

linux: OUTPUT = watchdogs
linux:
	$(CC) $(CFLAGS) -D__LINUX__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS)

termux: OUTPUT = watchdogs.tmux
termux:
	$(CC) $(CFLAGS) -D__ANDROID__ -D__W_VERSION__=\"$(FULL_VERSION)\" -fPIE $(SRCS) -o $(OUTPUT) $(LDFLAGS) -pie

windows: OUTPUT = watchdogs.win
windows:
	$(CC) -lshell32 -D_POSIX_C_SOURCE=200809L $(CFLAGS) $(SRCS) -D__WINDOWS32__ -D__W_VERSION__=\"$(FULL_VERSION)\" -o $(OUTPUT) $(LDFLAGS)

debug: DEBUG_MODE=1
debug: OUTPUT = watchdogs.debug
debug:
	$(CC) $(CFLAGS) -g -D_DBG_PRINT -D__LINUX__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS)

termux-debug: DEBUG_MODE=1
termux-debug: OUTPUT = watchdogs.debug.tmux
termux-debug:
	$(CC) $(CFLAGS) -g -D_DBG_PRINT -D__ANDROID__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS)

windows-debug: DEBUG_MODE=1
windows-debug: OUTPUT = watchdogs.debug.win
windows-debug:
	$(CC) -lshell32 -D_POSIX_C_SOURCE=200809L $(CFLAGS) -g -D_DBG_PRINT -D__WINDOWS32__ -D__W_VERSION__=\"$(FULL_VERSION)\" $(SRCS) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(OUTPUT) watchdogs watchdogs.win watchdogs.tmux \
	       watchdogs.debug watchdogs.debug.tmux watchdogs.debug.win
