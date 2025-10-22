# --- WATCHDOGS ---
VERSION  = WD-10.21
TARGET   ?= watchdogs
CC       ?= gcc

CFLAGS   = -Os -pipe -s -flto -fdata-sections -ffunction-sections -fPIE -D_GNU_SOURCE -I/usr/include/openssl
LDFLAGS  = -Wl,-O1,--gc-sections -lm -lcurl -ltinfo -lreadline -lncurses -larchive -lssl -lcrypto

YELLOW := \033[1;33m
RESET  := \033[0m

SRCS = chain.c hardware.c utils.c compiler.c archive.c curl.c package.c server.c crypto.c \
       tomlc99/toml.c cJSON/cJSON.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean linux termux mingw64 compress strip debug

all: $(TARGET)
	@echo "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)"
	@$(MAKE) compress

$(TARGET): $(OBJS)
	@echo "$(YELLOW)==>$(RESET) Linking $(TARGET)"
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@$(MAKE) strip

%.o: %.c
	@echo "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)"
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)==>$(RESET) Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

strip:
	@if [ -f "$(TARGET)" ]; then \
		echo "$(YELLOW)==>$(RESET) Stripping binary..."; \
		strip --strip-all $(TARGET) || true; \
	else \
		echo "$(YELLOW)==>$(RESET) Nothing to strip"; \
	fi

compress:
	@if command -v upx >/dev/null 2>&1; then \
		echo "$(YELLOW)==>$(RESET) Compressing with UPX..."; \
		upx --best --lzma $(TARGET) || true; \
	else \
		echo "$(YELLOW)==>$(RESET) UPX not found, skipping compression."; \
	fi

clean:
	rm -f $(OBJS) watchdogs watch_termux watchdogs.exe watchdogs.debug
	@echo "$(YELLOW)==>$(RESET) Clean done."

linux:
	@echo "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)"
	@$(MAKE) TARGET=watchdogs CC=gcc \
	CFLAGS="$(CFLAGS)" \
	LDFLAGS="$(LDFLAGS)"

termux:
	@echo "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)"
	@$(MAKE) TARGET=watch_termux CC=clang \
	CFLAGS="$(CFLAGS) -D__ANDROID__ -fPIE" \
	LDFLAGS="$(LDFLAGS) -pie"

mingw64:
	@echo "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)"
	@$(MAKE) TARGET=watchdogs.exe CC=x86_64-w64-mingw32-gcc \
	CFLAGS="$(CFLAGS) -DWIN32 -D_WIN32 -fno-pie" \
	LDFLAGS="-static -static-libgcc -static-libstdc++ -Wl,-O1,--gc-sections -lws2_32 -lwinmm"

debug:
	@echo "$(YELLOW)==>$(RESET) Building $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Build complete: $(TARGET) version $(VERSION)"
	@echo "$(YELLOW)==>$(RESET) Building DEBUG version with sanitizers"
	$(CC) -g -O0 -fno-omit-frame-pointer -fsanitize=address,undefined \
	-D_GNU_SOURCE -D_DBG_PRINT -I/usr/include/openssl \
	$(SRCS) -o watchdogs.debug \
	-lm -lcurl -ltinfo -lreadline -lncurses -larchive -lssl -lcrypto -pthread
	@echo "$(YELLOW)==>$(RESET) Debug build complete: ./watchdogs.debug"
