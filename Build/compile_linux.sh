set -e

echo "Select project to build:"
echo "1) Master"
echo "2) GameServer"
read -n1 -p "Project: " project_choice
echo ""

case "$project_choice" in
    1) PROJECT="Master" ;;
    2) PROJECT="GameServer" ;;
    *) echo "Invalid project type"
       exit 1 ;;
esac

echo ""
echo "Select mode to build:"
echo "1) Debug"
echo "2) Release"
read -n1 -p "Build Mode: " mode_choice
echo ""

case "$mode_choice" in
    1) MODE="Debug" ;;
    2) MODE="Release" ;;
    *) echo "Invalid build mode" 
       exit 1 ;;
esac    

BUILD_DIR="build_$PROJECT"
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo "Building for $PROJECT"
cmake ../../CMake -DCMAKE_BUILD_TYPE=$MODE -DSERVER_TO_BUILD=$PROJECT
cmake --build .
echo "$PROJECT build completed!"
