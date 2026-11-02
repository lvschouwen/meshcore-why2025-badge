#include "IdentityStore.h"

#include <string.h>

#if defined(ESP32)
// why2025-badge (#110): the identity is saved atomically. It is written to "<name>.id.new", read back and compared, and
// only then renamed over "<name>.id"; a cut write never replaces the only copy. FatFs can't rename over an existing
// file, so the shim removes the old one first: a power cut right there leaves only the verified ".new", which load()
// takes and renames into place. A ".new" that does not load is a save that never finished: ignored (and with no
// "<name>.id" beside it there never was an identity, so exists() says no and a new one may be made).
static void id_paths(const char *dir, const char *name, char *main_path, char *new_path) {
  sprintf(main_path, "%s/%s.id", dir, name);
  sprintf(new_path, "%s/%s.id.new", dir, name);
}

static bool load_file(FILESYSTEM *fs, const char *path, mesh::LocalIdentity &id) {
  bool loaded = false;
  if (fs->exists(path)) {
    File file = fs->open(path);
    if (file) {
      loaded = id.readFrom(file);
      file.close();
    }
  }
  return loaded;
}

static bool same_identity(mesh::LocalIdentity a, mesh::LocalIdentity b) {
  uint8_t x[PRV_KEY_SIZE + PUB_KEY_SIZE], y[PRV_KEY_SIZE + PUB_KEY_SIZE];
  size_t n = a.writeTo(x, sizeof(x));  // the private key is only reachable this way
  return n == sizeof(x) && b.writeTo(y, sizeof(y)) == n && memcmp(x, y, n) == 0;
}

static bool zero_key(mesh::LocalIdentity id) {
  uint8_t raw[PRV_KEY_SIZE];
  if (id.writeTo(raw, sizeof(raw)) != sizeof(raw)) return true;
  uint8_t any = 0;
  for (size_t i = 0; i < sizeof(raw); i++) any |= raw[i];
  return any == 0;
}

bool IdentityStore::exists(const char *name) {
  char path[48], tmp[48];
  mesh::LocalIdentity unused;
  id_paths(_dir, name, path, tmp);
  return _fs->exists(path) || load_file(_fs, tmp, unused);
}

bool IdentityStore::load(const char *name, mesh::LocalIdentity& id) {
  char path[48], tmp[48];
  id_paths(_dir, name, path, tmp);
  if (_fs->exists(path)) {
    return load_file(_fs, path, id);  // a file there that can't be read stays unread: never replaced by a ".new"
  }
  if (!load_file(_fs, tmp, id)) {
    return false;
  }
  _fs->rename(tmp, path);  // the cut save's verified copy; tried again next boot if this fails
  return true;
}

bool IdentityStore::save(const char *name, const mesh::LocalIdentity& id) {
  char path[48], tmp[48];
  id_paths(_dir, name, path, tmp);
  if (zero_key(id)) {
    MESH_DEBUG_PRINTLN("IdentityStore::save() refused: no private key");
    return false;  // what MyMesh holds when the file could not be read: never saved over anything
  }
  {
    File file = _fs->open(tmp, "w", true);
    if (!file) return false;
    bool written = id.writeTo(file);
    file.close();
    if (!written) {
      _fs->remove(tmp);
      return false;
    }
  }
  mesh::LocalIdentity check;
  if (!load_file(_fs, tmp, check) || !same_identity(id, check)) {
    _fs->remove(tmp);
    MESH_DEBUG_PRINTLN("IdentityStore::save() read-back differs");
    return false;
  }
  return _fs->rename(tmp, path);
}
#else
bool IdentityStore::exists(const char *name) {
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
  return _fs->exists(filename);
}

bool IdentityStore::load(const char *name, mesh::LocalIdentity& id) {
  bool loaded = false;
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
  if (_fs->exists(filename)) {
#if defined(RP2040_PLATFORM)
    File file = _fs->open(filename, "r");
#else
    File file = _fs->open(filename);
#endif
    if (file) {
      loaded = id.readFrom(file);
      file.close();
    }
  }
  return loaded;
}
#endif

bool IdentityStore::load(const char *name, mesh::LocalIdentity& id, char display_name[], int max_name_sz) {
  bool loaded = false;
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);
  if (_fs->exists(filename)) {
#if defined(RP2040_PLATFORM)
    File file = _fs->open(filename, "r");
#else
    File file = _fs->open(filename);
#endif
    if (file) {
      loaded = id.readFrom(file);

      int n = max_name_sz;   // up to 32 bytes
      if (n > 32) n = 32;
      file.read((uint8_t *) display_name, n);
      display_name[n - 1] = 0;  // ensure null terminator

      file.close();
    }
  }
  return loaded;
}

#if !defined(ESP32)
bool IdentityStore::save(const char *name, const mesh::LocalIdentity& id) {
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _fs->remove(filename);
  File file = _fs->open(filename, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(filename, "w");
#else
  File file = _fs->open(filename, "w", true);
#endif
  if (file) {
    bool success = id.writeTo(file);
    file.close();
    MESH_DEBUG_PRINTLN("IdentityStore::save() write - %s", success ? "OK" : "Err");
    return true;
  }
  MESH_DEBUG_PRINTLN("IdentityStore::save() failed");
  return false;
}
#endif

bool IdentityStore::save(const char *name, const mesh::LocalIdentity& id, const char display_name[]) {
  char filename[40];
  sprintf(filename, "%s/%s.id", _dir, name);

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _fs->remove(filename);
  File file = _fs->open(filename, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File file = _fs->open(filename, "w");
#else
  File file = _fs->open(filename, "w", true);
#endif
  if (file) {
    id.writeTo(file);

    uint8_t tmp[32];
    memset(tmp, 0, sizeof(tmp));
    int n = strlen(display_name);
    if (n > sizeof(tmp)-1) n = sizeof(tmp)-1;
    memcpy(tmp, display_name, n);
    file.write(tmp, sizeof(tmp));

    file.close();
    return true;
  }
  return false;
}
