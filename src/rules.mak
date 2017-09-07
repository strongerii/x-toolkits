
all: $(TARGET) 

$(TARGET): $(OBJS)
	$(RM) $@;
	@echo $(OBJS)
	$(LINK) $(CFLAGS) -o $(TARGET) $(OBJS) $(CLIB)
	
.c.o:
	$(CC)  $(CFLAGS) $(CINCLUDE) -c -o $@ $<
	@echo $@
.cpp.o:
	$(CPP) $(CFLAGS) $(CINCLUDE) -c -o $@ $<
	@echo $@

clean:
	rm -f $(TARGET) $(OBJS);

