SOURCES := diff_match_patch.cpp diff_match_patch_util.cpp

OBJDIR := objs

OBJECTS := $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp,%.o,$(SOURCES))))
OUT_LIB := libdiff_match_patch.a

CXXFLAGS := -std=c++17 \
	-g -O0

TEST_SRC := diff_match_patch_test.cpp
TEST_OBJECTS := $(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cpp,%.o,$(TEST_SRC))))
TEST_EXEC := test_diff_match_patch

DEPDIR := .d
DEPFLAGS = -MT $@ -MMD -MP -MF$(DEPDIR)/$*.Td

POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

ALL_SRC := $(SOURCES) $(TEST_SRC)

.PHONY: all clean init

all: init $(OUT_LIB) $(TEST_EXEC)

$(OBJDIR)/%.o : %.cpp %.h $(DEPDIR)/%.d
	$(CXX) $(DEPFLAGS) $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c -o "$@" "$<"
	$(POSTCOMPILE)

$(OUT_LIB) : $(OBJECTS)
	$(AR) -rv "$@" $(OBJECTS)

$(TEST_EXEC) : $(TEST_OBJECTS) $(OUT_LIB)
	$(CXX) $(LDFLAGS) -o "$@" "$<" $(OUT_LIB)

init:
	mkdir -p $(OBJDIR) $(DEPDIR)

clean:
	$(RM) -r $(OUT_LIB) $(OBJDIR) $(TEST_EXEC) $(DEPDIR)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d


include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(ALL_SRC))))
