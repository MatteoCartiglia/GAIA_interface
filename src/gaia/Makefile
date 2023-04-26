#---------------------------------------------------------------------
# MACRO DEFINITIONS
#---------------------------------------------------------------------

EXE = gaia_gui
IMGUI_DIR = ../../imgui
IMPLOT_DIR = ../../imgui
SOURCE_DIR = .
BACKENDS_DIR = ../../imgui/imgui_backend
OBJ_DIR = ./.obj
OBJ_PATH = ./.obj/

SOURCE_DATA_DIR = ../src/data
SOURCE_VISUALIZATION_DIR = ../src/visualization

OPENCV_DIR = /usr/local/include/opencv4
#---------------------------------------------------------------------
# COMMON BUILD FLAGS
#---------------------------------------------------------------------

UNAME_S := $(shell uname -s)

LINUX_GL_LIBS =  -lokFrontPanel

okFP_SDK ?= ./frontpanel

CXXFLAGS += -std=c++11 \
			-I$(IMGUI_DIR) \
			-I$(BACKENDS_DIR)\
			-I$(okFP_SDK) \
			-g -Wall -Wformat \
			-Wno-deprecated-declarations \
			-DEBUG

okFP_LIBS := -lokFrontPanel
LDFLAGS := -L$(okFP_SDK) -l boost_system 
LIBS := -ldl -lpthread


#---------------------------------------------------------------------
# INPUT SOURCES
#---------------------------------------------------------------------
SOURCES +=  $(SOURCE_DIR)/main.cpp \
			$(IMGUI_DIR)/imgui.cpp \
			$(IMGUI_DIR)/imgui_demo.cpp \
			$(IMGUI_DIR)/imgui_draw.cpp \
			$(IMGUI_DIR)/imgui_tables.cpp \
			$(IMGUI_DIR)/imgui_widgets.cpp \
			$(IMPLOT_DIR)/implot.cpp \
			$(IMPLOT_DIR)/implot_items.cpp \
			$(IMPLOT_DIR)/implot_demo.cpp \
			$(BACKENDS_DIR)/imgui_impl_opengl3.cpp \
			$(BACKENDS_DIR)/imgui_impl_glfw.cpp \
			$(IMGUI_DIR)/ImGuiFileDialog.cpp \
			# $(SOURCE_DATA_DIR)/bioamp.cpp \
			$(SOURCE_DATA_DIR)/events.cpp \
			$(SOURCE_DATA_DIR)/bias_control.cpp \
			$(SOURCE_VISUALIZATION_DIR)/gaia_visualization.cpp \
			$(SOURCE_VISUALIZATION_DIR)/visualize_adc_channel.cpp \
			$(SOURCE_VISUALIZATION_DIR)/bias_control.cpp 


OBJS = $(addprefix $(OBJ_PATH), $(notdir $(SOURCES:.cpp=.o)))

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3` -ldl -lpthread 

	CXXFLAGS += `pkg-config --cflags -lfw3` -lstdc++fs -lGL
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib
	LIBS += -lglfw $(LINUX_GL_LIBS)
	LIBS += `pkg-config --libs opencv4`
	LIBS += `pkg-config --static --libs glfw3` -ldl -lpthread
	CXXFLAGS += -I/usr/local/include
	CXXFLAGS += `pkg-config --cflags opencv4`

	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMPLOT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(BACKENDS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<


all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(DEBUG) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)