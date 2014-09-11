
all: $(TARGET) 

$(TARGET): $(OBJS)
	$(RM) $@;
	$(LINK) $(CFLAGS) -o $(TARGET) $(OBJS) $(CLIB)
#	$(LINK) $(TARGET) $(OBJS) 
.c.o:
	$(CC)  $(CFLAGS) $(CINCLUDE) -c -o $@ $<
	@echo $@
.cpp.o:
	$(CPP) $(CFLAGS) $(CINCLUDE) -c -o $@ $<
	@echo $@
install:$(TARGET)
	install $(TARGET) $(INSTALLDIR);
clean:
	rm -f $(TARGET) $(OBJS);

