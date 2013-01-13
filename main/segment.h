/* $Id: segment.h,v 1.1.1.1 2006/03/17 19:55:56 zicodxx Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Include file for functions which need to access segment data structure.
 *
 */

#ifndef _SEGMENT_H
#define _SEGMENT_H

#include <physfs.h>
#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "object.types.h"
#include "dxxerror.h"

#ifdef __cplusplus
#include <array>
#include <boost/serialization/strong_typedef.hpp>

// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define SIDE_IS_QUAD    1   // render side as quadrilateral
#define SIDE_IS_TRI_02  2   // render side as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13  3   // render side as two triangles, triangulated along edge from 1 to 3

// Set maximum values for segment and face data structures.
#define MAX_VERTICES_PER_SEGMENT    8
#define MAX_SIDES_PER_SEGMENT       6
#define MAX_VERTICES_PER_POLY       4
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

#define MAX_SEGMENTS_ORIGINAL         900u
#define MAX_SEGMENT_VERTICES_ORIGINAL (4*MAX_SEGMENTS_ORIGINAL)
#define MAX_SEGMENTS                  9000u
#define MAX_SEGMENT_VERTICES          (4*MAX_SEGMENTS)

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (F1_0/2)

#ifdef EDITOR   //verts for the new segment
# define NUM_NEW_SEG_VERTICES   8
# define NEW_SEGMENT_VERTICES   (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES+NUM_NEW_SEG_VERTICES)
#else           //No editor
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES)
#endif

typedef unsigned vertnum_t;

#define DECLARE_SEGMENT_INDEX(N,V)	DECLARE_TYPESAFE_INDEX(segment,N,V)

DECLARE_SEGMENT_INDEX(first, 0);
DECLARE_SEGMENT_INDEX(exit, 0xfffe);
DECLARE_SEGMENT_INDEX(none, 0xffff);

struct Highest_segment_index_t
{
	unsigned contained_value;
	void operator=(const unsigned & rhs) { contained_value = rhs; }
	operator const unsigned & () const { return contained_value; }
};

struct Num_segments_t
{
	unsigned contained_value;
	void operator=(const unsigned & rhs) { contained_value = rhs; }
	operator const unsigned&() const { return contained_value; }
	Num_segments_t& operator++(int) { contained_value++; return *this; }
	Num_segments_t& operator--(int) { contained_value--; return *this; }
};

#ifdef DXX_USE_STRICT_TYPESAFE
/*
 * This is based on BOOST_STRONG_TYPEDEF, but that macro does not permit
 * sufficient customization for the required use cases.
 */
struct segnum_t
	: boost::totally_ordered1< segnum_t
		, boost::equality_comparable2< segnum_t, segment_exit_type_t
			, boost::equality_comparable2< segnum_t, segment_none_type_t
			>
		>
	>
{
	enum
	{
		segment_first = typesafe_idx_segment::first,
		segment_exit = typesafe_idx_segment::exit,
		segment_none = typesafe_idx_segment::none,
	};
	unsigned short contained_value;
	explicit segnum_t(const unsigned short& t_) : contained_value(t_) {};
	segnum_t() = default;
	// segnum_t & operator=(const unsigned short & rhs) { contained_value = rhs; return *this; }
	segnum_t & operator=(const Highest_segment_index_t & rhs) { contained_value = rhs.contained_value; return *this; }
	operator unsigned short () const { return contained_value; }
	// operator unsigned short & () { return contained_value; }
	bool operator==(const segnum_t & rhs) const { return contained_value == rhs.contained_value; }
	bool operator<(const segnum_t & rhs) const { return (contained_value < rhs.contained_value); }
	segnum_t& operator++() { ++ contained_value; return *this; }
	segnum_t& operator++(int) { contained_value ++; return *this; }
	segnum_t& operator--(int) { contained_value --; return *this; }
	DEFINE_CONSTRUCT_SPECIAL(segnum_t, segment_first);
	DEFINE_CONSTRUCT_SPECIAL(segnum_t, segment_exit);
	DEFINE_CONSTRUCT_SPECIAL(segnum_t, segment_none);
	DEFINE_COMPARE_SPECIAL(==, segment_exit);
	DEFINE_COMPARE_SPECIAL(==, segment_none);
	DEFINE_COMPARE_PASSTHROUGH(<=, Highest_segment_index_t);
	DEFINE_COMPARE_PASSTHROUGH(>, Highest_segment_index_t);
	DEFINE_COMPARE_PASSTHROUGH(<, Num_segments_t);
	DEFINE_COMPARE_PASSTHROUGH(>=, Num_segments_t);
	template <typename T> segnum_t & operator=(T) = delete;
	template <typename T> bool operator==(T) const = delete;
	template <typename T> bool operator!=(T) const = delete;
	template <typename T> bool operator<=(T) const = delete;
	template <typename T> bool operator>=(T) const = delete;
	template <typename T> bool operator<(T) const = delete;
	template <typename T> bool operator>(T) const = delete;
};
#else
typedef unsigned short segnum_t;
#endif

//typedef unsigned short vertnum_t;

// Returns true if segnum references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
static inline int IS_CHILD(segnum_t s) {
	return s != segment_exit && s != segment_none;
}

template <typename T> int IS_CHILD(T) DXX_CXX11_EXPLICIT_DELETE;

//Structure for storing u,v,light values.
//NOTE: this structure should be the same as the one in 3d.h
typedef struct uvl {
	fix u, v, l;
} uvl;

typedef struct side_t {
#ifdef COMPACT_SEGS
	sbyte   type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	ubyte   pad;            //keep us longword alligned
	short   wall_num;
	short   tmap_num;
	short   tmap_num2;
	uvl     uvls[4];
	//vms_vector normals[2];  // 2 normals, if quadrilateral, both the same.
#else
	sbyte   type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	ubyte   pad;            //keep us longword alligned
	short   wall_num;
	short   tmap_num;
	short   tmap_num2;
	uvl     uvls[4];
	vms_vector normals[2];  // 2 normals, if quadrilateral, both the same.
#endif
} side_t;

static inline int IS_WALL(short w) {
	return w > -1;
}

typedef struct segment {
#ifdef EDITOR
	segnum_t   segnum;     // segment number, not sure what it means
#endif
	side_t    sides[MAX_SIDES_PER_SEGMENT];       // 6 sides
	segnum_t   children[MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	vertnum_t     verts[MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
#ifdef EDITOR
	short   group;      // group number to which the segment belongs.
#endif
	objnum_t objects;    // pointer to objects in this segment
	int     degenerated; // true if this segment has gotten turned inside out, or something.

	// -- Moved to segment2 to make this struct 512 bytes long --
	//ubyte   special;    // what type of center this is
	//sbyte   matcen_num; // which center segment is associated with.
	//short   value;
	//fix     static_light; //average static light in segment
	//#ifndef EDITOR
	//short   pad;        //make structure longword aligned
	//#endif
} segment;

#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02

enum segment_type_t : ubyte
{
//values for special field
	SEGMENT_IS_NOTHING = 0,
	SEGMENT_IS_FUELCEN = 1,
	SEGMENT_IS_REPAIRCEN = 2,
	SEGMENT_IS_CONTROLCEN = 3,
	SEGMENT_IS_ROBOTMAKER = 4,
	SEGMENT_IS_GOAL_BLUE = 5,
	SEGMENT_IS_GOAL_RED = 6,
	MAX_CENTER_TYPES = 7,
};

typedef struct segment2 {
	segment_type_t   special;
	sbyte   matcen_num;
	sbyte   value;
	ubyte   s2_flags;
	fix     static_light;
} segment2;

#ifdef COMPACT_SEGS
void get_side_normal(segment *sp, int sidenum, int normal_num, vms_vector * vm );
void get_side_normals(segment *sp, int sidenum, vms_vector * vm1, vms_vector *vm2 );
#endif

// Local segment data.
// This is stuff specific to a segment that does not need to get
// written to disk.  This is a handy separation because we can add to
// this structure without obsoleting existing data on disk.

#define SS_REPAIR_CENTER    0x01    // Bitmask for this segment being part of repair center.

//--repair-- typedef struct {
//--repair-- 	int     special_type;
//--repair-- 	short   special_segment; // if special_type indicates repair center, this is the base of the repair center
//--repair-- } lsegment;

typedef std::array<segnum_t, MAX_SEGMENTS> group_segment_array_t;

typedef struct {
	unsigned     num_segments;
	unsigned     num_vertices;
	group_segment_array_t   segments;
	vertnum_t    vertices[MAX_VERTICES];
} group;

template <typename T>
struct segment_array_template_t
{
	typedef std::array<T, MAX_SEGMENTS> array_t;
	array_t a;
#ifdef DXX_USE_STRICT_TYPESAFE
	typename array_t::reference operator[](segment_first_type_t) { return a[segnum_t::segment_first]; }
	template <typename U> void operator[](U) = delete;
#endif
	typename array_t::reference operator[](const segnum_t& s) { Assert(s < size()); return a[s]; }
#ifdef EDITOR
	/*
	 * This special case is required to allow some defined(EDITOR) code
	 * to work.
	 */
	typename array_t::reference operator[](const Highest_segment_index_t& s) { return a[s.contained_value]; }
#endif
	segnum_t idx(typename array_t::const_pointer p) const
	{
		return segnum_t(std::distance(&*a.begin(), p));
	}
	typename array_t::reference back() { return a.back(); }
	Num_segments_t size() const {
		Num_segments_t n;
		n.contained_value = (unsigned)a.size();
		return n;
	}
	void fill(typename array_t::const_reference v) { a.fill(v); }
};

template <typename T>
static inline segnum_t operator-(const T *p, const segment_array_template_t<T>& a)
{
	return a.idx(p);
}

typedef segment_array_template_t<segment> segment_array_t;
typedef segment_array_template_t<segment2> segment2_array_t;
typedef segment_array_template_t<ubyte> automap_visited_array_t;

// Globals from mglobal.c
extern vms_vector   Vertices[MAX_VERTICES];
extern segment_array_t      Segments;
extern segment2_array_t     Segment2s;
extern Num_segments_t          Num_segments;
extern unsigned          Num_vertices;

// Get pointer to the segment2 for the given segment pointer
#define s2s2(segp) (&Segment2s[(segp) - Segments])

extern const sbyte Side_to_verts[MAX_SIDES_PER_SEGMENT][4];       // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const int  Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4];    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const char Side_opposite[MAX_SIDES_PER_SEGMENT];                                // Side_opposite[my_side] returns side opposite cube from my_side.

#define SEG_PTR_2_NUM(segptr) (Assert((unsigned) (segptr-Segments)<MAX_SEGMENTS),(segptr)-Segments)

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in segnum:sidenum by some light
typedef struct {
	segnum_t   segnum;
	sbyte   sidenum;
	sbyte   dummy;
	ubyte   vert_light[4];
} delta_light;

// Light at segnum:sidenum casts light on count sides beginning at index (in array Delta_lights)
typedef struct {
	segnum_t   segnum;
	sbyte   sidenum;
	sbyte   count;
	short   index;
} dl_index;

#define MAX_DL_INDICES      500
#define MAX_DELTA_LIGHTS    10000

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

extern dl_index     Dl_indices[MAX_DL_INDICES];
extern delta_light  Delta_lights[MAX_DELTA_LIGHTS];
extern int          Num_static_lights;

int subtract_light(segnum_t segnum, int sidenum);
int add_light(segnum_t segnum, int sidenum);
void restore_all_lights_in_mine(void);
void clear_light_subtracted(void);

typedef segment_array_template_t<ubyte> light_subtracted_array_t;
extern light_subtracted_array_t	Light_subtracted;

// ----------------------------------------------------------------------------
// --------------------- Segment interrogation functions ----------------------
// Do NOT read the segment data structure directly.  Use these
// functions instead.  The segment data structure is GUARANTEED to
// change MANY TIMES.  If you read the segment data structure
// directly, your code will break, I PROMISE IT!

// Return a pointer to the list of vertex indices for the current
// segment in vp and the number of vertices in *nv.
void med_get_vertex_list(segment *s,int *nv,vertnum_t **vp);

// Return a pointer to the list of vertex indices for face facenum in
// vp and the number of vertices in *nv.
void med_get_face_vertex_list(segment *s,int side, int facenum,int *nv,int **vp);

// Set *nf = number of faces in segment s.
void med_get_num_faces(segment *s,int *nf);

void med_validate_segment_side(segment *sp,int side);

// Delete segment from group
void delete_segment_from_group(segnum_t segment_num, int group_num);

// Add segment to group
void add_segment_to_group(segnum_t segment_num, int group_num);

// Verify that all vertices are legal.
void med_check_all_vertices();

/*
 * reads a segment2 structure from a PHYSFS_file
 */
void segment2_read(segment2 *s2, PHYSFS_file *fp);

/*
 * reads a delta_light structure from a PHYSFS_file
 */
void delta_light_read(delta_light *dl, PHYSFS_file *fp);

/*
 * reads a dl_index structure from a PHYSFS_file
 */
void dl_index_read(dl_index *di, PHYSFS_file *fp);

void segment2_write(segment2 *s2, PHYSFS_file *fp);
void delta_light_write(delta_light *dl, PHYSFS_file *fp);
void dl_index_write(dl_index *di, PHYSFS_file *fp);

#endif

#endif
