##### LIBRARIES CONFIGURATION #####

SYSTEMC := ../../systemc-2.3.1

##### DIRECTORIES #####

SYSTEMC_LIBS := $(wildcard $(SYSTEMC)/lib-*)

SRCDIR  := ./
OBJDIR  := ./build

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(subst $(SRCDIR),$(OBJDIR),$(SRCS:.cpp=.o))
DEPS := $(OBJS:.o=.d)

MODULE := TaskMapping

##### COMPILER OPTIONS #####

CXX      := g++
OPT      := -O3 -MMD
DEBUG    := 
OTHER    := -Wall# -Wno-deprecated
CXXFLAGS := $(OPT) $(OTHER) $(DEBUG)

INCDIR := -I$(SRCDIR) -isystem $(SYSTEMC)/include
LIBDIR := -L$(SRCDIR) -L$(SYSTEMC_LIBS)

LIBS := -lsystemc -lm

##### RULES #####

$(MODULE): $(OBJS)
	@echo "Linking: $@"
	@$(CXX) $(LIBDIR) $^ $(LIBS) -o $@ 2>&1 | c++filt
	@echo "Linking was successful."

-include $(DEPS)
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling: $<"
	@$(CXX) $(CXXFLAGS) $(INCDIR) -c $< -o $@

clean:
	@echo "Removing object files..."
	@rm -f $(OBJS) $(MODULE) $(DEPS) *~
	@rm -rf $(OBJDIR)
	@echo "Removing was successful."

