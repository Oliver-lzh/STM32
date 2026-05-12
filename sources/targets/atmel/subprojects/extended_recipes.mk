

$(info **** Build Type: $(BUILD_TYPE))


# the existing recipe has to be replicated to support parent directories in build

$(build-dir)%.o: ../../../../%.c $(MAKEFILE_PATH) config.mk
	$(Q)test -d $(dir $@) || echo $(MSG_MKDIR)
ifeq ($(os),Windows)
	$(Q)test -d $(patsubst %/,%,$(dir $@)) || mkdir $(subst /,\,$(dir $@))
else
	$(Q)test -d $(dir $@) || mkdir -p $(dir $@)
endif
	@echo $(MSG_COMPILING)
	$(Q)$(CC) $(c_flags) -c $< -o $@

$(build-dir)%.o: ../../%.c $(MAKEFILE_PATH) config.mk
	$(Q)test -d $(dir $@) || echo $(MSG_MKDIR)
ifeq ($(os),Windows)
	$(Q)test -d $(patsubst %/,%,$(dir $@)) || mkdir $(subst /,\,$(dir $@))
else
	$(Q)test -d $(dir $@) || mkdir -p $(dir $@)
endif
	@echo $(MSG_COMPILING)
	$(Q)$(CC) $(c_flags) -c $< -o $@
