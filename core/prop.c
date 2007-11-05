/*
 * Copyright (c) 2002-2007 Hypertriton, Inc. <http://hypertriton.com/>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Implementation of generic properties for AG_Object.
 */

#include <config/have_ieee754.h>

#include <core/core.h>

#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

const AG_Version agPropTblVer = { 2, 0 };

#ifdef DEBUG
#define DEBUG_STATE	0x01
#define DEBUG_SET	0x02

int	agPropDebugLvl = 0;
#define agDebugLvl agPropDebugLvl
#endif

#if 0
AG_PropOps *agPropOps = NULL;
Uint        agPropOpsCnt = 0;

void
AG_PropRegister(const AG_PropOps *ops)
{
	agPropOps = Realloc(agPropOps, (agPropOpsCnt+1)*sizeof(AG_PropOps *));
	agPropOps[agPropOpsCnt++] = ops;
}
#endif

/* Return the copy of a property. */
AG_Prop *
AG_CopyProp(const AG_Prop *prop)
{
	AG_Prop *nprop;

	nprop = Malloc(sizeof(AG_Prop));
	memcpy(nprop, prop, sizeof(AG_Prop));

	switch (prop->type) {
	case AG_PROP_STRING:
		nprop->data.s = Strdup(prop->data.s);
		if (nprop->data.s == NULL) {
			Free(nprop);
			AG_SetError(_("Out of memory for string"));
			return (NULL);
		}
		break;
	default:
		break;
	}
	return (0);
}

#undef PROP_SET
#define PROP_SET(fn,v,type,argtype) do { \
	if (prop->writeFn.fn != NULL) { \
		prop->data.v = (type)prop->writeFn.fn(ob, prop, \
		    (type)va_arg(ap, argtype)); \
	} else { \
		prop->data.v = (type)va_arg(ap, argtype); \
	} \
} while (0)

/* Create a new property, or modify an existing one. */
AG_Prop *
AG_SetProp(void *p, const char *key, enum ag_prop_type type, ...)
{
	AG_Object *ob = p;
	AG_Prop *prop;
	int modify = 0;
	va_list ap;

	TAILQ_FOREACH(prop, &ob->props, props) {
		if (prop->type == type &&
		    strcmp(prop->key, key) == 0)
			break;
	}
	if (prop == NULL) {
		prop = Malloc(sizeof(AG_Prop));
		strlcpy(prop->key, key, sizeof(prop->key));
		prop->type = type;
		prop->writeFn.wUint = NULL;
		prop->readFn.rUint = NULL;
		debug_n(DEBUG_SET, "%s: (new) => ", key);
	} else {
		modify++;
	}

	va_start(ap, type);
	switch (type) {
	case AG_PROP_UINT:	PROP_SET(wUint, u, Uint, Uint);		break;
	case AG_PROP_INT:	PROP_SET(wUint, i, int, int);		break;
	case AG_PROP_BOOL:	PROP_SET(wBool, i, int, int);		break;
	case AG_PROP_FLOAT:	PROP_SET(wFloat, f, float, double);	break;
	case AG_PROP_DOUBLE:	PROP_SET(wDouble, d, double, double);	break;
#ifdef HAVE_LONG_DOUBLE
	case AG_PROP_LONG_DOUBLE:
		PROP_SET(wLongDouble, ld, long double, long double);
		break;
#endif
	case AG_PROP_STRING:
		if (modify) {
			char *old_s = prop->data.s;
			PROP_SET(wString, s, char *, char *);
			if (prop->data.s != old_s) { Free(old_s); }
		} else {
			PROP_SET(wString, s, char *, char *);
		}
		break;
	case AG_PROP_POINTER:	PROP_SET(wPointer, p, void *, void *);	break;
	case AG_PROP_UINT8:	PROP_SET(wUint8, u8, Uint8, int);	break;
	case AG_PROP_SINT8:	PROP_SET(wSint8, s8, Sint8, int);	break;
	case AG_PROP_UINT16:	PROP_SET(wUint16, u16, Uint16, int);	break;
	case AG_PROP_SINT16:	PROP_SET(wSint16, s16, Sint16, int);	break;
	case AG_PROP_UINT32:	PROP_SET(wUint32, u32, Uint32, int);	break;
	case AG_PROP_SINT32:	PROP_SET(wSint32, s32, Sint32, int);	break;
#ifdef HAVE_64BIT
	case AG_PROP_UINT64:	PROP_SET(wUint64, u64, Uint64, int);	break;
	case AG_PROP_SINT64:	PROP_SET(wSint64, s64, Sint64, int);	break;
#endif
	default:
		fatal("bad prop type: %d", type);
	}
	va_end(ap);

	AG_MutexLock(&ob->lock);
	if (!modify) {
		TAILQ_INSERT_TAIL(&ob->props, prop, props);
		AG_PostEvent(NULL, ob, "prop-added", "%p", prop);
	} else {
		AG_PostEvent(NULL, ob, "prop-modified", "%p", prop);
	}
	AG_MutexUnlock(&ob->lock);
	return (prop);
}
#undef PROP_SET

#undef PROP_GET
#define PROP_GET(fn,v,type) do { \
	if (prop->readFn.fn != NULL) { \
		if (p != NULL) { \
			*(type *)p = prop->readFn.fn(ob, prop); \
		} else { \
			prop->data.v = prop->readFn.fn(ob, prop); \
		} \
	} else { \
		if (p != NULL) \
			*(type *)p = (type)prop->data.v; \
	} \
} while (0)

/* Return a pointer to the data of a property. */
AG_Prop *
AG_GetProp(void *obp, const char *key, int t, void *p)
{
	AG_Object *ob = obp;
	AG_Prop *prop;

	AG_MutexLock(&ob->lock);
	TAILQ_FOREACH(prop, &ob->props, props) {
		if ((t >= 0 && t != prop->type) ||
		    strcmp(key, prop->key) != 0) {
			continue;
		}
		switch (prop->type) {
		case AG_PROP_INT: 	PROP_GET(rInt, i, int);		break;
		case AG_PROP_BOOL:	PROP_GET(rBool, i, int);	break;
		case AG_PROP_UINT:	PROP_GET(rUint, u, unsigned);	break;
		case AG_PROP_UINT8:	PROP_GET(rUint8, u8, Uint8);	break;
		case AG_PROP_SINT8:	PROP_GET(rSint8, s8, Sint8);	break;
		case AG_PROP_UINT16:	PROP_GET(rUint16, u16, Uint16);	break;
		case AG_PROP_SINT16:	PROP_GET(rSint16, s16, Sint16);	break;
		case AG_PROP_UINT32:	PROP_GET(rUint32, u32, Uint32);	break;
		case AG_PROP_SINT32:	PROP_GET(rSint32, s32, Sint32);	break;
#ifdef HAVE_64BIT
		case AG_PROP_UINT64:	PROP_GET(rUint64, u64, Uint64);	break;
		case AG_PROP_SINT64:	PROP_GET(rSint64, s64, Sint64);	break;
#endif
		case AG_PROP_FLOAT:	PROP_GET(rFloat, f, float);	break;
		case AG_PROP_DOUBLE:	PROP_GET(rDouble, d, double);	break;
#ifdef HAVE_LONG_DOUBLE
		case AG_PROP_LONG_DOUBLE:
			PROP_GET(rLongDouble, ld, long double);
			break;
#endif
		case AG_PROP_STRING:	PROP_GET(rString, s, char *);	break;
		case AG_PROP_POINTER:	PROP_GET(rPointer, p, void *);	break;
		default:
			AG_SetError("bad prop %d", prop->type);
			goto fail;
		}
		AG_MutexUnlock(&ob->lock);
		return (prop);
	}
	AG_SetError(_("%s: no such property: `%s' (%d)."), ob->name, key, t);
fail:
	AG_MutexUnlock(&ob->lock);
	return (NULL);
}

/* Search for a property referenced by a "object-name:prop-name" string. */
AG_Prop *
AG_FindProp(const char *spec, int type, void *rval)
{
	char sb[AG_OBJECT_PATH_MAX+1+AG_PROP_KEY_MAX];
	char *s = &sb[0], *objname, *propname;
	void *obj;

	strlcpy(sb, spec, sizeof(sb));
	objname = AG_Strsep(&s, ":");
	propname = AG_Strsep(&s, ":");
	if (objname == NULL || propname == NULL ||
	    objname[0] == '\0' || propname[0] == '\0') {
		AG_SetError(_("Invalid property path: `%s'"), spec);
		return (NULL);
	}
	if ((obj = AG_ObjectFind(objname)) == NULL) {
		return (NULL);
	}
	return (AG_GetProp(obj, propname, -1, rval));
}

int
AG_PropPath(char *dst, size_t size, const void *obj, const char *prop_name)
{
	if (AG_ObjectCopyName(obj, dst, size) == -1 ||
	    strlcat(dst, ":", size) >= size ||
	    strlcat(dst, prop_name, size) >= size) {
		AG_SetError("String overflow");
		return (-1);
	}
	return (0);
}

int
AG_PropLoad(void *p, AG_DataSource *ds)
{
	AG_Object *ob = p;
	Uint32 i, nprops;

	if (AG_ReadVersion(ds, "AG_PropTbl", &agPropTblVer, NULL) == -1)
		return (-1);

	AG_MutexLock(&ob->lock);

	if ((ob->flags & AG_OBJECT_RELOAD_PROPS) == 0)
		AG_ObjectFreeProps(ob);

	nprops = AG_ReadUint32(ds);
	for (i = 0; i < nprops; i++) {
		char key[AG_PROP_KEY_MAX];
		Uint32 t;

		if (AG_CopyString(key, ds, sizeof(key)) >= sizeof(key)) {
			AG_SetError("key %lu >= %lu", (Ulong)strlen(key),
			    (Ulong)sizeof(key));
			goto fail;
		}
		t = AG_ReadUint32(ds);
		
		switch (t) {
		case AG_PROP_BOOL:
			AG_SetBool(ob, key, (int)AG_ReadUint8(ds));
			break;
		case AG_PROP_UINT8:
			AG_SetBool(ob, key, AG_ReadUint8(ds));
			break;
		case AG_PROP_SINT8:
			AG_SetBool(ob, key, AG_ReadSint8(ds));
			break;
		case AG_PROP_UINT16:
			AG_SetUint16(ob, key, AG_ReadUint16(ds));
			break;
		case AG_PROP_SINT16:
			AG_SetSint16(ob, key, AG_ReadSint16(ds));
			break;
		case AG_PROP_UINT32:
			AG_SetUint32(ob, key, AG_ReadUint32(ds));
			break;
		case AG_PROP_SINT32:
			AG_SetSint32(ob, key, AG_ReadSint32(ds));
			break;
#ifdef HAVE_64BIT
		case AG_PROP_UINT64:
			AG_SetUint64(ob, key, AG_ReadUint64(ds));
			break;
		case AG_PROP_SINT64:
			AG_SetSint64(ob, key, AG_ReadSint64(ds));
			break;
#endif
		case AG_PROP_UINT:
			AG_SetUint(ob, key, (Uint)AG_ReadUint32(ds));
			break;
		case AG_PROP_INT:
			AG_SetInt(ob, key, (int)AG_ReadSint32(ds));
			break;
#ifdef HAVE_IEEE754
		case AG_PROP_FLOAT:
			AG_SetFloat(ob, key, AG_ReadFloat(ds));
			break;
		case AG_PROP_DOUBLE:
			AG_SetDouble(ob, key, AG_ReadDouble(ds));
			break;
# ifdef HAVE_LONG_DOUBLE
		case AG_PROP_LONG_DOUBLE:
			AG_SetDouble(ob, key, AG_ReadLongDouble(ds));
			break;
# endif
#endif
		case AG_PROP_STRING:
			{
				char *s;

				s = AG_ReadString(ds);
#ifdef AG_PROP_STRING_LIMIT
				if (strlen(s) >= AG_PROP_STRING_LIMIT) {
					AG_SetError("prop string %lu >= %lu",
					    strlen(s), AG_PROP_STRING_LIMIT);
					Free(s);
					goto fail;
				}
#endif
				AG_SetString(ob, key, "%s", s);
				Free(s);
			}
			break;
		default:
			AG_SetError(_("Cannot load property of type %d."), 
			    (int)t);
			goto fail;
		}
	}
	AG_MutexUnlock(&ob->lock);
	return (0);
fail:
	AG_MutexUnlock(&ob->lock);
	return (-1);
}

int
AG_PropSave(void *p, AG_DataSource *ds)
{
	AG_Object *ob = p;
	off_t count_offs;
	Uint32 nprops = 0;
	AG_Prop *prop;
	Uint8 c;
	
	AG_WriteVersion(ds, "AG_PropTbl", &agPropTblVer);
	
	AG_MutexLock(&ob->lock);

	count_offs = AG_Tell(ds);			/* Skip count */
	AG_WriteUint32(ds, 0);

	TAILQ_FOREACH(prop, &ob->props, props) {
		AG_WriteString(ds, (char *)prop->key);
		AG_WriteUint32(ds, prop->type);
		debug(DEBUG_STATE, "%s -> %s\n", ob->name, prop->key);
		switch (prop->type) {
		case AG_PROP_BOOL:
			c = (prop->data.i == 1) ? 1 : 0;
			AG_WriteUint8(ds, c);
			break;
		case AG_PROP_UINT8:
			AG_WriteUint8(ds, prop->data.u8);
			break;
		case AG_PROP_SINT8:
			AG_WriteSint8(ds, prop->data.s8);
			break;
		case AG_PROP_UINT16:
			AG_WriteUint16(ds, prop->data.u16);
			break;
		case AG_PROP_SINT16:
			AG_WriteSint16(ds, prop->data.s16);
			break;
		case AG_PROP_UINT32:
			AG_WriteUint32(ds, prop->data.u32);
			break;
		case AG_PROP_SINT32:
			AG_WriteSint32(ds, prop->data.s32);
			break;
#ifdef HAVE_64BIT
		case AG_PROP_UINT64:
			AG_WriteUint64(ds, prop->data.u64);
			break;
		case AG_PROP_SINT64:
			AG_WriteSint64(ds, prop->data.s64);
			break;
#endif
		case AG_PROP_UINT:
			AG_WriteUint32(ds, (Uint32)prop->data.u);
			break;
		case AG_PROP_INT:
			AG_WriteSint32(ds, (Sint32)prop->data.i);
			break;
#ifdef HAVE_IEEE754
		case AG_PROP_FLOAT:
			AG_WriteFloat(ds, prop->data.f);
			break;
		case AG_PROP_DOUBLE:
			AG_WriteDouble(ds, prop->data.d);
			break;
# ifdef HAVE_LONG_DOUBLE
		case AG_PROP_LONG_DOUBLE:
			AG_WriteLongDouble(ds, prop->data.ld);
			break;
# endif
#endif
		case AG_PROP_STRING:
			AG_WriteString(ds, prop->data.s);
			break;
		case AG_PROP_POINTER:
			break;
		default:
			AG_SetError(_("Property of type %d cannot be saved."),
			    prop->type);
			goto fail;
		}
		nprops++;
	}
	AG_MutexUnlock(&ob->lock);
	AG_WriteUint32At(ds, nprops, count_offs);	/* Write count */
	return (0);
fail:
	AG_MutexUnlock(&ob->lock);
	return (-1);
}

void
AG_PropDestroy(AG_Prop *prop)
{
	switch (prop->type) {
	case AG_PROP_STRING:
		Free(prop->data.s);
		break;
	}
}

void
AG_PropPrint(char *s, size_t len, void *obj, const char *pname)
{
	AG_Prop *pr;

	pr = AG_GetProp(obj, pname, -1, NULL);
	if (pr == NULL) {
		strlcpy(s, "(?prop)", len);
		return;
	}
	switch (pr->type) {
	case AG_PROP_UINT:	snprintf(s, len, "%u", pr->data.u);	break;
	case AG_PROP_INT:	snprintf(s, len, "%d", pr->data.i);	break;
	case AG_PROP_UINT8:	snprintf(s, len, "%u", pr->data.u8);	break;
	case AG_PROP_SINT8:	snprintf(s, len, "%d", pr->data.s8);	break;
	case AG_PROP_UINT16:	snprintf(s, len, "%u", pr->data.u16);	break;
	case AG_PROP_SINT16:	snprintf(s, len, "%d", pr->data.s16);	break;
	case AG_PROP_UINT32:
		snprintf(s, len, "%lu", (unsigned long)pr->data.u32);
		break;
	case AG_PROP_SINT32:
		snprintf(s, len, "%ld", (long)pr->data.s32);
		break;
#ifdef HAVE_64BIT
	case AG_PROP_UINT64:	snprintf(s, len, "%llu",
				(unsigned long long)pr->data.u64);
				break;
	case AG_PROP_SINT64:	snprintf(s, len, "%lld",
				(long long)pr->data.s64);
				break;
#endif	
	case AG_PROP_FLOAT:	snprintf(s, len, "%f", pr->data.f);	break;
	case AG_PROP_DOUBLE:	snprintf(s, len, "%f", pr->data.d);	break;
#ifdef HAVE_LONG_DOUBLE
	case AG_PROP_LONG_DOUBLE: snprintf(s, len, "%Lf", pr->data.ld); break;
#endif
	case AG_PROP_STRING:	snprintf(s, len, "\"%s\"", pr->data.s);	break;
	case AG_PROP_POINTER:	snprintf(s, len, "->%p", pr->data.p);	break;
	case AG_PROP_BOOL:
		strlcpy(s, pr->data.i ? "true" : "false", len);
		break;
	default:
		strlcat(s, "(?prop-type)", len);
		break;
	}
}

size_t
AG_StringCopy(void *p, const char *key, char *buf, size_t bufsize)
{
	size_t sl;
	char *s;
	AG_LockProps(p);
	if (AG_GetProp(p, key, AG_PROP_STRING, &s) == NULL) {
		AG_FatalError("%s", AG_GetError());
	}
	sl = strlcpy(buf, s, bufsize);
	AG_UnlockProps(p);
	return (sl);
}

size_t
AG_FindStringCopy(const char *key, char *buf, size_t bufsize)
{
	size_t sl;
	char *s;

	/* XXX thread unsafe */
	if (AG_FindProp(key, AG_PROP_STRING, &s) == NULL) {
		AG_FatalError("%s", AG_GetError());
	}
	sl = strlcpy(buf, s, bufsize);
	return (sl);
}

AG_Prop *
AG_SetString(void *ob, const char *key, const char *fmt, ...)
{
	va_list ap;
	char *s;

	va_start(ap, fmt);
	Vasprintf(&s, fmt, ap);
	va_end(ap);
	return (AG_SetProp(ob, key, AG_PROP_STRING, s));
}
