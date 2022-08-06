#pragma pack(push, 1)
typedef struct {
  SBYTE signature[2];
  UDWORD vertexcount;
  UDWORD facecount;
} SMHEADER;
#pragma pack(pop)

typedef struct {
  SMHEADER header;
  UBYTE unused0[6];
  VERTEX *vertices;
  UDWORD *indices;
} SMODEL;

#pragma pack(push, 1)
typedef struct {
  SBYTE signature[2];
  UDWORD modelcount;
  UDWORD objectcount;
} SWHEADER;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  UWORD modelid;
  UWORD textureid; // TODO
  VECTOR3F position;
  VECTOR3F rotation;
} SWOBJECT;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  UWORD modelid;
} SWMODEL;
#pragma pack(pop)

typedef struct {
  SWHEADER header;
  UBYTE unused0[6];
  SWMODEL *models;
  SWOBJECT *objects;
} SWORLD;