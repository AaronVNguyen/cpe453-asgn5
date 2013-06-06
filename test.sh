IMAGE="../asgn5/images/TestImage"
P_IMAGE="../asgn5/images/Partitioned"

make clean
make

echo "minls TestImage"
./minls $IMAGE
echo ""

echo "minls -p 0 Partitioned"
./minls -p 0 $P_IMAGE
echo ""

echo "minls -v TestImage"
./minls -v $IMAGE
echo ""

echo "minls TestImage /"
./minls $IMAGE /
echo ""

echo "minls TestImage /Hello"
./minls $IMAGE /Hello
echo ""

echo "minls TestImage /src"
./minls $IMAGE /src
echo ""
