// Converter from old Warzone (savegame) map format to new format.

#include "maplib.h"

#include <set>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/QImage>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "mapload.h"

#define GRDLANDVERSION	4
#define	ELEVATION_SCALE	2
#define GRAVITY		1
#define SEALEVEL	0
#define TILE_NUMMASK	0x01ff
#define TILE_XFLIP	0x8000
#define TILE_YFLIP	0x4000
#define TILE_ROTMASK	0x3000
#define TILE_ROTSHIFT	12
#define TILE_TRIFLIP	0x0800
#define TRI_FLIPPED(x)	((x)->texture & TILE_TRIFLIP)
#define SNAP_MODE	0

#define DEG(degrees) ((degrees) * 8192 / 45)

static const char *tilesetTextures[] = { "arizona", "urban", "rockies" };

int main(int argc, char **argv)
{
	char filename[PATH_MAX], *p_filename, *base;
	GAMEMAP *map;
	FILE *fp;

	if (argc != 2)
	{
		printf("Usage: %s <map>\n", argv[0]);
		return -1;
	}
	
	physfs_init(argv[0]);
	strcpy(filename, physfs_addmappath(argv[1]));

	map = mapLoad(filename);
	if (!map)
	{
		fprintf(stderr, "Failed to load map %s\n", filename);
		return -1;
	}
	
	p_filename = strrchr(filename, '/');
	if (p_filename)
	{
		p_filename++;
		base = strdup(p_filename);
	}
	else
	{
		base = strdup(filename);
	}
	if (!PHYSFS_exists(base))
	{
		PHYSFS_mkdir(base);
	}
	physfs_shutdown();
	QJsonArray decals;
	printf("Converting %s (version %d)\n", base, map->mapVersion);

	/*** Terrain data ***/
	if (map->mapVersion > 0)
	{
		const std::set<int> arizona_decals = { 37, 47, 49, 50, 51, 52, 55, 56, 57, 58, 59, 62, 63, 64, 65, 66, 67, 68, 69, 72, 73 };
		const std::set<int> urban_decals = { 1, 5, 21, 28, 36, 40, 41, 42, 43, 44, 45, 46, 47, 52, 55, 56, 57 };
		const std::set<int> rockies_decals = { 1, 8, 13, 27, 28, 37, 43, 47, 49, 50, 51, 52, 56, 57, 58, 59, 60, 62, 70, 72, 73, 74, 75 };
		uint8_t *terrainmap = new uint8_t[map->width * map->height];
		uint8_t *heightmap = new uint8_t[map->width * 2 * map->height * 2];

		memset(heightmap, 0, map->width * 2 * map->height * 2);
		for (int i = 0; i < map->width * map->height; i++)
		{
			char decal[PATH_MAX];
			const int x = i % map->width;
			const int y = i / map->width;
			const MAPTILE *psTile = mapTile(map, x, y);
			const int height = psTile->height;
			const int right_height = (x == map->width - 1) ? height : mapTile(map, x + 1, y)->height;
			const int bottom_height = (y == map->height - 1) ? height : mapTile(map, x, y + 1)->height;
			const int texture = psTile->texture & TILE_NUMMASK;

			heightmap[y * 2 * map->width * 2 + x * 2 + 0] = height;
			heightmap[y * 2 * map->width * 2 + x * 2 + 1] = height;
			heightmap[(y * 2 + 1) * map->width * 2 + x * 2 + 0] = (height + right_height) / 2; // interpolated middle
			heightmap[(y * 2 + 1) * map->width * 2 + x * 2 + 1] = (height + bottom_height) / 2;

			terrainmap[i] = terrainType(psTile) * 16; // multiply here to make color nuances trivially visible to naked eye
			const uint16_t rotate = ((psTile->texture & TILE_ROTMASK) >> TILE_ROTSHIFT) * 90;
			const uint8_t flip = TRI_FLIPPED(psTile) ? 1 : 0;

			if (map->tileset == TILESET_ARIZONA && std::find(arizona_decals.begin(), arizona_decals.end(), texture) != arizona_decals.end())
			{
				sprintf(decal, "tertilesc1hw-128/tile-%02d.png", texture);
			}
			else if (map->tileset == TILESET_URBAN && std::find(urban_decals.begin(), urban_decals.end(), texture) != urban_decals.end())
			{
				sprintf(decal, "tertilesc2hw-128/tile-%02d.png", texture);
			}
			else if (map->tileset == TILESET_ROCKIES && std::find(rockies_decals.begin(), rockies_decals.end(), texture) != rockies_decals.end())
			{
				sprintf(decal, "tertilesc3hw-128/tile-%02d.png", texture);
			}
			else
			{
				continue; // no decal
			}

			// Write decal info
			QJsonObject v;
			v["x"] = (int)(i % map->width);
			v["y"] = (int)(i / map->height);
			v["decal"] = decal;
			v["flip"] = (bool)flip;
			v["rotate"] = (int)rotate;
			decals.append(v);
		}
		QJsonDocument mJson;
		QJsonObject mObj = mJson.object();
		mObj["decals"] = decals;
		strcpy(filename, base);
		strcat(filename, "/decals.json");
		printf("writing %s\n", filename);
		fp = fopen(filename, "w");
		if (!fp) printf("%s: %s\n", filename, strerror(errno));
		QJsonDocument doc(mObj);
		QByteArray json = doc.toJson();
		fwrite(json.constData(), 1, json.size(), fp);
		fclose(fp);

		strcpy(filename, base);
		strcat(filename, "/height.png");
		unlink(filename);
		QImage himage(heightmap, map->width * 2, map->height * 2, QImage::Format_Grayscale8);
		himage.save(filename);

		strcpy(filename, base);
		strcat(filename, "/terrain.png");
		unlink(filename);
		QImage timage(terrainmap, map->width, map->height, QImage::Format_Grayscale8);
		timage.save(filename);

		delete [] heightmap;
		delete [] terrainmap;
	}

	/*** Gateways ***/
	if (map->mapVersion > 0)
	{
		QJsonArray gateways;
		for (int i = 0; i < map->numGateways; i++)
		{
			const GATEWAY *psGate = mapGateway(map, i);
			QJsonObject v;

			v["x1"] = (int)psGate->x1;
			v["y1"] = (int)psGate->y1;
			v["x2"] = (int)psGate->x2;
			v["y2"] = (int)psGate->y2;
			gateways.append(v);
		}
		QJsonDocument mJson;
		QJsonObject mObj = mJson.object();
		mObj["gateways"] = gateways;
		mObj["width"] = (int)map->width;
		mObj["height"] = (int)map->height;
		mObj["tileset"] = tilesetTextures[map->tileset];
		mObj["version"] = 2; // version 1 uses .gam + json
		QJsonObject v;
		v["x1"] = (int)map->scrollMinX;
		v["y1"] = (int)map->scrollMinY;
		v["x2"] = (int)map->scrollMaxX;
		v["y2"] = (int)map->scrollMaxY;
		mObj["scroll_limits"] = v;
		if (map->levelName[0] == '\0')
		{
			mObj["name"] = base;
		}
		else
		{
			mObj["name"] =  map->levelName;
		}
		switch (map->gameType)
		{
		case 0: mObj["gametype"] = "Start"; break;
		case 1: mObj["gametype"] = "Expand"; break;
		case 2: mObj["gametype"] = "Mission"; break;
		case 3: mObj["gametype"] = "Autosave"; break;
		case 4: mObj["gametype"] = "Savegame"; break;
		default: fprintf(stderr, "%s: Bad gametype %d", filename, map->gameType); break;
		}
		QJsonArray players;
		bool writeout = false;
		for (int i = 0; i < 8; i++)
		{
			QJsonObject v;
			v["index"] = i;
			// Read power information. Note for Start maps, this is later overwritten by scripts.
			// Which is incidentially why it works even though we now have more than 8 players and
			// read unmodified maps with only 8 power data values.
			if (map->power[i] > 0)
			{
				mObj["power"] = (int)map->power[i];
				writeout = true;
			}
			players.append(v);
		}
		if (writeout) // only write out if we have any useful data
		{
			mObj["players"] = players;
		}
		strcpy(filename, base);
		strcat(filename, "/mapinfo.json");
		printf("writing %s\n", filename);
		fp = fopen(filename, "w");
		if (!fp) printf("%s: %s\n", filename, strerror(errno));
		QJsonDocument doc(mObj);
		QByteArray json = doc.toJson();
		fwrite(json.constData(), 1, json.size(), fp);
		fclose(fp);
	}

	mapFree(map);

	return 0;
}
