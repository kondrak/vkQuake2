typedef struct
{
	char id[4]; // Should be 'PACK'
	int dstart; // Offest in the file to the directory
	int dsize;  // Size in bytes of the directory, same as num_items*64
} pak_header_t;

typedef struct
{
	char name[56]; // The name of the item, normal C string
	int start; // Offset in .pak file to start of item
	int size; // Size of item in bytes
} pak_item_t;
