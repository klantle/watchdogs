UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
  ESC := $(shell printf '\033')
  YELLOW := $(ESC)[1;33m
  RESET := $(ESC)[0m
else ifeq ($(UNAME_S),MINGW64_NT-10.0)
  ESC := $(shell printf '\033')
  YELLOW := $(ESC)[1;33m
  RESET := $(ESC)[0m
else
  YELLOW :=
  RESET :=
endif

export LC_ALL := C.UTF-8
export LANG := C.UTF-8

VERSION  	 = WD-10.21
FULL_VERSION = WD-25.10.21.0
TARGET   	 ?= watchdogs
CC       	 ?= gcc

CFLAGS   = -Os -pipe -s -flto -fdata-sections -ffunction-sections -fPIE -I/usr/include/openssl
LDFLAGS  = -Wl,-O1,--gc-sections -lm -lcurl -lreadline -lncurses -larchive -lssl -lcrypto

SRCS = extra.c chain.c utils.c hardware.c compiler.c archive.c curl.c package.c server.c crypto.c \
       tomlc99/toml.c cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: install all clean linux termux windows compress strip debug windows-debug

install:
	@echo "$(YELLOW)==>$(RESET) Detecting system environment..."
	@UNAME_S=$$(uname -s); \
	if echo "$$UNAME_S" | grep -qi "Linux" && [ -d "/data/data/com.termux" ]; then \
		echo "$(YELLOW)==>$(RESET) Detected: Termux environment"; \
		echo "$(YELLOW)==>$(RESET) Installing required packages..."; \
		pkg update -y && \
		pkg install -y clang openssl curl libarchive ncurses readline; \
		echo "$(YELLOW)==>$(RESET) Building project..."; \
		$(MAKE) termux; \
		chmod +x watchdogs_termux; \
		echo "$(YELLOW)==>$(RESET) Installation complete. Run with ./watchdogs_termux"; \
	elif echo "$$UNAME_S" | grep -qi "MINGW64_NT"; then \
		echo "$(YELLOW)==>$(RESET) Detected: MSYS2 MinGW UCRT64 environment"; \
		echo "$(YELLOW)==>$(RESET) Installing required packages via pacman..."; \
		pacman -Sy --noconfirm; \
		pacman -S --needed --noconfirm \
			base-devel \
			mingw-w64-ucrt-x86_64-toolchain \
			mingw-w64-ucrt-x86_64-curl \
			mingw-w64-ucrt-x86_64-readline \
			mingw-w64-ucrt-x86_64-ncurses \
			mingw-w64-ucrt-x86_64-libarchive \
			mingw-w64-ucrt-x86_64-openssl \
			mingw-w64-ucrt-x86_64-upx; \
		echo "$(YELLOW)==>$(RESET) Building project..."; \
		$(MAKE) windows; \
		chmod +x watchdogs.win; \
		echo "$(YELLOW)==>$(RESET) Installation complete. Run with ./watchdogs.win"; \
	elif echo "$$UNAME_S" | grep -qi "Linux"; then \
		echo "$(YELLOW)==>$(RESET) Detected: Linux environment"; \
		echo "$(YELLOW)==>$(RESET) Installing required packages via apt..."; \
		sudo apt update -y && \
		sudo apt install -y build-essential libssl-dev libcurl4-openssl-dev \
			libncurses5-dev libreadline-dev libarchive-dev; \
		echo "$(YELLOW)==>$(RESET) Building project..."; \
		$(MAKE) linux; \
		chmod +x watchdogs; \
		echo "$(YELLOW)==>$(RESET) Installation complete. Run with ./watchdogs"; \
	else \
		echo "$(YELLOW)==>$(RESET) Unknown or unsupported environment."; \
		echo "Please install dependencies manually."; \
		exit 1; \
	fi

all: $(TARGET)
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@$(MAKE) compress

$(TARGET): $(OBJS)
	@printf "$(YELLOW)==>$(RESET) Linking $(TARGET)\n"
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@$(MAKE) strip

%.o: %.c
	@printf "$(YELLOW)==>$(RESET) Compiling $<\n"
	$(CC) $(CFLAGS) -c $< -o $@

strip:
	@if [ -f "$(TARGET)" ]; then \
		printf "$(YELLOW)==>$(RESET) Stripping binary...\n"; \
		strip --strip-all $(TARGET) || true; \
	else \
		printf "$(YELLOW)==>$(RESET) Nothing to strip\n"; \
	fi

compress:
	@if command -v upx >/dev/null 2>&1; then \
		printf "$(YELLOW)==>$(RESET) Compressing with UPX...\n"; \
		upx --best --lzma $(TARGET) || true; \
	else \
		printf "$(YELLOW)==>$(RESET) UPX not found, skipping compression.\n"; \
	fi

clean:
	rm -f $(OBJS) watchdogs watchdogs_termux watchdogs.win watchdogs.debug atchdogs.debug.win
	@printf "$(YELLOW)==>$(RESET) Clean done.\n"

linux:
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@$(MAKE) TARGET=watchdogs CC=gcc \
	CFLAGS="$(CFLAGS)" \
	LDFLAGS="$(LDFLAGS)"

termux:
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	@$(MAKE) TARGET=watchdogs_termux CC=clang \
	CFLAGS="$(CFLAGS) -D__ANDROID__ -fPIE" \
	LDFLAGS="$(LDFLAGS) -pie"

windows: $(OBJS)
	@printf "$(YELLOW)==>$(RESET) Building $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) $(CFLAGS) $(OBJS) -o watchdogs.win \
		$(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Build complete: $(TARGET) Version $(VERSION) Full Version $(FULL_VERSION)\n"

windows-debug: $(OBJS)
	@printf "$(YELLOW)==>$(RESET) Building DEBUG Version $(VERSION) Full Version $(FULL_VERSION)\n"
	$(CC) -g -O0 -D_DBG_PRINT -Wall \
	$(OBJS) -o atchdogs.debug.win \
	$(LDFLAGS) -liphlpapi -lshlwapi
	@printf "$(YELLOW)==>$(RESET) Debug build complete: ./atchdogs.debug.win\n"