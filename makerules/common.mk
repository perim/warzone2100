%.o: %.rc
	$(WINDRES) -o$@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o$@ $<

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o$@ $<

%.c: %.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

%.c: %.y
	$(BISON) -d $(BISONFLAGS) -o$@ $<
