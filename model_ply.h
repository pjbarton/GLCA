class Model_PLY
{
public:
	int Load(char *filename);
	
	Model_PLY();

	float* Vertices;
	float* FaceVertices;

	int NumVertices;
	int NumFaces;
};

