all: put_stream put_file rs_manger prefetch refresh
SOURCE_FILES=\
	../b64/urlsafe_b64.c\
	../cJSON/cJSON.c\
	../efs/conf.c\
	../efs/base.c\
	../efs/base_io.c\
	../efs/http.c\
	../efs/auth_mac.c\
	../efs/rs.c\
	../efs/io.c\
	../efs/resumable_io.c\
	../efs/fop.c\
	../efs/region.c\
	../efs/reader.c\
	../efs/tm.c

PUT_STREAM=$(SOURCE_FILES)\
	put_stream.c
PUT_FILE=$(SOURCE_FILES)\
	put_file.c
RS_MANGER=$(SOURCE_FILES)\
	resource_manger.c
PREFETCH=$(SOURCE_FILES)\
	../efs/cdn.c\
	cdn_prefetch.c
REFRESH=$(SOURCE_FILES)\
	../efs/cdn.c\
	cdn_refresh.c
put_stream: $(PUT_STREAM)
	gcc -g $^ -o put_stream  -lcurl -lssl -lcrypto -lm
put_file: $(PUT_FILE)
	gcc -g $^ -o put_file  -lcurl -lssl -lcrypto -lm
rs_manger: $(RS_MANGER)
	gcc -g $^ -o rs_manger  -lcurl -lssl -lcrypto -lm
prefetch: $(PREFETCH)
	gcc -g $^ -o prefetch  -lcurl -lssl -lcrypto -lm
refresh: $(REFRESH)
	gcc -g $^ -o refresh  -lcurl -lssl -lcrypto -lm
install: all
	@echo

clean:
	rm -f put_stream put_file rs_manger prefetch refresh

