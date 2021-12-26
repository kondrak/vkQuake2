
enum {
	TEXT_LEFT,
	TEXT_CENTER,
	TEXT_RIGHT
};

typedef struct texthnd_s {
	struct text_s *lines;
	int nlines;
	int curline;
	int size;
	int	flags;
	int	allocated;
	float last_update;
	byte *buffer;
} texthnd_t;

typedef struct text_s {
	char *text;
} text_t;

void Text_Open(edict_t *ent);
void Text_Close(edict_t *ent);
void Text_Update(edict_t *ent);
void Text_Next(edict_t *ent);
void Text_Prev(edict_t *ent);

