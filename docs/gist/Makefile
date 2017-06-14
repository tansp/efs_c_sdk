CLIENT_SOURCE_FILES=\
	../../b64/urlsafe_b64.c\
	../../cJSON/cJSON.c\
	../../efs/conf.c\
	../../efs/base.c\
	../../efs/base_io.c\
	../../efs/http.c\
	../../efs/io.c\
	../../efs/resumable_io.c\
	client.c\

SERVER_SOURCE_FILES=\
	../../b64/urlsafe_b64.c\
	../../cJSON/cJSON.c\
	../../efs/conf.c\
	../../efs/base.c\
	../../efs/base_io.c\
	../../efs/http.c\
	../../efs/auth_mac.c\
	../../efs/rs.c\
	../../efs/io.c\
	server.c\

all: client server
	@echo "OK"

client: $(CLIENT_SOURCE_FILES)
	gcc $^ -o efsclient.exe -lcurl -lm

server: $(SERVER_SOURCE_FILES)
	gcc $^ -o efsserver.exe -lcurl -lssl -lcrypto -lm

install: all
	@echo

clean:
	rm -f *.exe

test:
	@echo

