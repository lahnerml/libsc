/*
  This file is part of p4est.
  p4est is a C library to manage a collection (a forest) of multiple
  connected adaptive quadtrees or octrees in parallel.

  Copyright (C) 2010 The University of Texas System
  Written by Carsten Burstedde, Lucas C. Wilcox, and Tobin Isaac

  p4est is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  p4est is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with p4est; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifdef P4_TO_P8
#include <p8est_connectivity.h>
#else
#include <p4est_connectivity.h>
#endif
#include <sc_io.h>
#ifdef P4EST_METIS
#include <metis.h>
#endif

#ifndef P4_TO_P8

/* *INDENT-OFF* */
const int           p4est_face_corners[4][2] =
{{ 0, 2 },
 { 1, 3 },
 { 0, 1 },
 { 2, 3 }};
const int           p4est_face_dual[4] = { 1, 0, 3, 2 };

const int           p4est_corner_faces[4][2] =
{{ 0, 2 },
 { 1, 2 },
 { 0, 3 },
 { 1, 3 }};
const int           p4est_corner_face_corners[4][4] =
{{  0, -1,  0, -1 },
 { -1,  0,  1, -1 },
 {  1, -1, -1,  0 },
 { -1,  1, -1,  1 }};

const int           p4est_child_corner_faces[4][4] =
{{ -1,  2,  0, -1 },
 {  2, -1, -1,  1 },
 {  0, -1, -1,  3 },
 { -1,  1,  3, -1 }};
/* *INDENT-ON* */

#endif /* !P4_TO_P8 */

size_t
p4est_connectivity_memory_used (p4est_connectivity_t * conn)
{
  return sizeof (p4est_connectivity_t) +
    (conn->num_vertices > 0 ?
     (conn->num_vertices * 3 * sizeof (double) +
      conn->num_trees * P4EST_CHILDREN * sizeof (p4est_topidx_t)) : 0) +
    conn->num_trees * P4EST_FACES * (sizeof (p4est_topidx_t) +
                                     sizeof (int8_t)) +
#ifdef P4_TO_P8
    conn->num_trees * P8EST_EDGES * sizeof (p4est_topidx_t) +
    (conn->num_edges + 1) * sizeof (p4est_topidx_t) +
    conn->ett_offset[conn->num_edges] * (sizeof (p4est_topidx_t) +
                                         sizeof (int8_t)) +
#endif
    conn->num_trees * P4EST_CHILDREN * sizeof (p4est_topidx_t) +
    (conn->num_corners + 1) * sizeof (p4est_topidx_t) +
    conn->ctt_offset[conn->num_corners] * (sizeof (p4est_topidx_t) +
                                           sizeof (int8_t));
}

p4est_connectivity_t *
p4est_connectivity_new_copy (p4est_topidx_t num_vertices,
                             p4est_topidx_t num_trees,
#ifdef P4_TO_P8
                             p4est_topidx_t num_edges,
#endif
                             p4est_topidx_t num_corners,
                             const double *vertices,
                             const p4est_topidx_t * ttv,
                             const p4est_topidx_t * ttt, const int8_t * ttf,
#ifdef P4_TO_P8
                             const p4est_topidx_t * tte,
                             const p4est_topidx_t * eoff,
                             const p4est_topidx_t * ett, const int8_t * ete,
#endif
                             const p4est_topidx_t * ttc,
                             const p4est_topidx_t * coff,
                             const p4est_topidx_t * ctt, const int8_t * ctc)
{
#ifdef P4_TO_P8
  p4est_topidx_t      num_ett;
#endif
  p4est_topidx_t      num_ctt;
  p4est_connectivity_t *conn;

#ifdef P4_TO_P8
  num_ett = eoff[num_edges];
#endif
  num_ctt = coff[num_corners];
  conn = p4est_connectivity_new (num_vertices, num_trees,
#ifdef P4_TO_P8
                                 num_edges, num_ett,
#endif
                                 num_corners, num_ctt);

  if (num_vertices > 0) {
    P4EST_ASSERT (vertices != NULL && ttv != NULL);
    memcpy (conn->vertices, vertices, sizeof (double) * 3 * num_vertices);
    memcpy (conn->tree_to_vertex, ttv,
            sizeof (p4est_topidx_t) * P4EST_CHILDREN * num_trees);
  }
  else {
    conn->vertices = NULL;
    conn->tree_to_vertex = NULL;
  }
  memcpy (conn->tree_to_tree, ttt,
          sizeof (p4est_topidx_t) * P4EST_FACES * num_trees);
  memcpy (conn->tree_to_face, ttf, sizeof (int8_t) * P4EST_FACES * num_trees);

#ifdef P4_TO_P8
  if (num_edges > 0) {
    memcpy (conn->tree_to_edge, tte,
            sizeof (p4est_topidx_t) * P8EST_EDGES * num_trees);
    memcpy (conn->edge_to_tree, ett, sizeof (p4est_topidx_t) * num_ett);
    memcpy (conn->edge_to_edge, ete, sizeof (int8_t) * num_ett);
  }
  memcpy (conn->ett_offset, eoff, sizeof (p4est_topidx_t) * (num_edges + 1));
#endif

  if (num_corners > 0) {
    memcpy (conn->tree_to_corner, ttc,
            sizeof (p4est_topidx_t) * P4EST_CHILDREN * num_trees);
    memcpy (conn->corner_to_tree, ctt, sizeof (p4est_topidx_t) * num_ctt);
    memcpy (conn->corner_to_corner, ctc, sizeof (int8_t) * num_ctt);
  }
  memcpy (conn->ctt_offset, coff,
          sizeof (p4est_topidx_t) * (num_corners + 1));

  P4EST_ASSERT (p4est_connectivity_is_valid (conn));

  return conn;
}

p4est_connectivity_t *
p4est_connectivity_new (p4est_topidx_t num_vertices, p4est_topidx_t num_trees,
#ifdef P4_TO_P8
                        p4est_topidx_t num_edges, p4est_topidx_t num_ett,
#endif
                        p4est_topidx_t num_corners, p4est_topidx_t num_ctt)
{
  p4est_connectivity_t *conn;

  conn = P4EST_ALLOC_ZERO (p4est_connectivity_t, 1);

  conn->num_vertices = num_vertices;
  conn->num_trees = num_trees;
  if (num_vertices > 0) {
    conn->vertices = P4EST_ALLOC (double, 3 * num_vertices);
    conn->tree_to_vertex =
      P4EST_ALLOC (p4est_topidx_t, P4EST_CHILDREN * num_trees);
  }
  else {
    conn->vertices = NULL;
    conn->tree_to_vertex = NULL;
  }
  conn->tree_to_tree = P4EST_ALLOC (p4est_topidx_t, P4EST_FACES * num_trees);
  conn->tree_to_face = P4EST_ALLOC (int8_t, P4EST_FACES * num_trees);

#ifdef P4_TO_P8
  conn->num_edges = num_edges;
  if (num_edges > 0) {
    conn->tree_to_edge =
      P4EST_ALLOC (p4est_topidx_t, P8EST_EDGES * num_trees);
    conn->edge_to_tree = P4EST_ALLOC (p4est_topidx_t, num_ett);
    conn->edge_to_edge = P4EST_ALLOC (int8_t, num_ett);
  }
  else {
    conn->tree_to_edge = NULL;
    conn->edge_to_tree = NULL;
    conn->edge_to_edge = NULL;
  }
  conn->ett_offset = P4EST_ALLOC (p4est_topidx_t, num_edges + 1);
  conn->ett_offset[num_edges] = num_ett;
#endif

  conn->num_corners = num_corners;
  if (num_corners > 0) {
    conn->tree_to_corner =
      P4EST_ALLOC (p4est_topidx_t, P4EST_CHILDREN * num_trees);
    conn->corner_to_tree = P4EST_ALLOC (p4est_topidx_t, num_ctt);
    conn->corner_to_corner = P4EST_ALLOC (int8_t, num_ctt);
  }
  else {
    conn->tree_to_corner = NULL;
    conn->corner_to_tree = NULL;
    conn->corner_to_corner = NULL;
  }
  conn->ctt_offset = P4EST_ALLOC (p4est_topidx_t, num_corners + 1);
  conn->ctt_offset[num_corners] = num_ctt;

  return conn;
}

void
p4est_connectivity_destroy (p4est_connectivity_t * conn)
{
  P4EST_FREE (conn->vertices);
  P4EST_FREE (conn->tree_to_vertex);

  P4EST_FREE (conn->tree_to_tree);
  P4EST_FREE (conn->tree_to_face);

#ifdef P4_TO_P8
  P4EST_FREE (conn->tree_to_edge);
  P4EST_FREE (conn->ett_offset);
  P4EST_FREE (conn->edge_to_tree);
  P4EST_FREE (conn->edge_to_edge);
#endif

  P4EST_FREE (conn->tree_to_corner);
  P4EST_FREE (conn->ctt_offset);
  P4EST_FREE (conn->corner_to_tree);
  P4EST_FREE (conn->corner_to_corner);

  P4EST_FREE (conn);
}

int
p4est_connectivity_is_valid (p4est_connectivity_t * conn)
{
  int                 nvert;
  int                 face, rface, nface, orientation;
  int                 errcode, errcount;
#ifdef P4_TO_P8
  int                 edge, nedge;
  int                 flip, nflip, nflip1, nflip2;
  p4est_topidx_t      aedge, edge_begin, edge_end;
  p4est_topidx_t      nett;
#endif
  int                 corner, ncorner;
  int                 good, cfound;
  p4est_topidx_t      vertex, tree, ntree;
  p4est_topidx_t      acorner, corner_begin, corner_end;
  p4est_topidx_t      nctt;
  const p4est_topidx_t num_vertices = conn->num_vertices;
  const p4est_topidx_t num_trees = conn->num_trees;
  const p4est_topidx_t *ttv = conn->tree_to_vertex;
  const p4est_topidx_t *ttt = conn->tree_to_tree;
  const int8_t       *ttf = conn->tree_to_face;
#ifdef P4_TO_P8
  const p4est_topidx_t num_edges = conn->num_edges;
  const p4est_topidx_t *tte = conn->tree_to_edge;
  const p4est_topidx_t *eoff = conn->ett_offset;
  const p4est_topidx_t *ett = conn->edge_to_tree;
  const int8_t       *ete = conn->edge_to_edge;
  const p4est_topidx_t num_ett = eoff[num_edges];
  p8est_edge_info_t   ei;
  sc_array_t         *eta = &ei.edge_transforms;
#endif
  const p4est_topidx_t num_corners = conn->num_corners;
  const p4est_topidx_t *ttc = conn->tree_to_corner;
  const p4est_topidx_t *coff = conn->ctt_offset;
  const p4est_topidx_t *ctt = conn->corner_to_tree;
  const int8_t       *ctc = conn->corner_to_corner;
  const p4est_topidx_t num_ctt = coff[num_corners];
  p4est_corner_info_t ci;
  sc_array_t         *cta = &ci.corner_transforms;

  good = 0;
#ifdef P4_TO_P8
  sc_array_init (eta, sizeof (p8est_edge_transform_t));
#endif
  sc_array_init (cta, sizeof (p4est_corner_transform_t));

  if (num_vertices == 0 && (conn->vertices != NULL || ttv != NULL)) {
    P4EST_NOTICE ("Zero vertices still with arrays\n");
    goto failure;
  }
  if (num_vertices > 0 && (conn->vertices == NULL || ttv == NULL)) {
    P4EST_NOTICE ("Nonzero vertices missing arrays\n");
    goto failure;
  }

#ifdef P4_TO_P8
  for (nett = 0; nett < num_ett; ++nett) {
    if (ett[nett] < 0 || ett[nett] >= num_trees) {
      P4EST_NOTICEF ("Edge to tree %lld out of range\n", (long long) nett);
      goto failure;
    }
    if (ete[nett] < 0 || ete[nett] >= 24) {
      P4EST_NOTICEF ("Edge to edge %lld out of range\n", (long long) nett);
      goto failure;
    }
  }
#endif

  for (nctt = 0; nctt < num_ctt; ++nctt) {
    if (ctt[nctt] < 0 || ctt[nctt] >= num_trees) {
      P4EST_NOTICEF ("Corner to tree %lld out of range\n", (long long) nctt);
      goto failure;
    }
    if (ctc[nctt] < 0 || ctc[nctt] >= P4EST_CHILDREN) {
      P4EST_NOTICEF ("Corner to corner %lld out of range\n",
                     (long long) nctt);
      goto failure;
    }
  }

  if (num_vertices > 0) {
    for (tree = 0; tree < num_trees; ++tree) {
      for (nvert = 0; nvert < P4EST_CHILDREN; ++nvert) {
        vertex = ttv[tree * P4EST_CHILDREN + nvert];
        if (vertex < 0 || vertex >= num_vertices) {
          P4EST_NOTICEF ("Tree to vertex out of range %lld %d",
                         (long long) tree, nvert);
          goto failure;
        }
      }
    }
  }

  for (tree = 0; tree < num_trees; ++tree) {
    for (face = 0; face < P4EST_FACES; ++face) {
      ntree = ttt[tree * P4EST_FACES + face];
      if (ntree < 0 || ntree >= num_trees) {
        P4EST_NOTICEF ("Tree to tree out of range %lld %d\n",
                       (long long) tree, face);
        goto failure;
      }
      rface = (int) ttf[tree * P4EST_FACES + face];
      if (rface < 0 || rface >= P4EST_FACES * P4EST_HALF) {
        P4EST_NOTICEF ("Tree to face out of range %lld %d\n",
                       (long long) tree, face);
        goto failure;
      }
      nface = rface % P4EST_FACES;      /* clamp to a real face index */
      orientation = rface / P4EST_FACES;        /* 0..P4EST_HALF-1 */
      if (ntree == tree) {
        /* no neighbor across this face or self-periodic */
        if (nface == face && orientation != 0) {
          P4EST_NOTICEF ("Face invalid in %lld %d\n", (long long) tree, face);
          goto failure;
        }
      }
      if (ntree != tree || nface != face) {
        /* check reciprocity */
        if (ttt[ntree * P4EST_FACES + nface] != tree) {
          P4EST_NOTICEF ("Tree to tree reciprocity in %lld %d\n",
                         (long long) tree, face);
          goto failure;
        }
        if ((int) ttf[ntree * P4EST_FACES + nface] !=
            face + P4EST_FACES * orientation) {
          P4EST_NOTICEF ("Tree to face reciprocity in %lld %d\n",
                         (long long) tree, face);
          goto failure;
        }
      }
    }

#ifdef P4_TO_P8
    if (num_edges > 0) {
      for (edge = 0; edge < P8EST_EDGES; ++edge) {
        p8est_find_edge_transform (conn, tree, edge, &ei);
        aedge = tte[tree * P8EST_EDGES + edge];
        if (aedge < -1 || aedge >= num_edges) {
          P4EST_NOTICEF ("Tree to edge out of range %lld %d\n",
                         (long long) tree, edge);
          goto failure;
        }
        if (aedge == -1) {
          continue;
        }
        errcode = errcount = 0;
        flip = nflip1 = nflip2 = -1;
        edge_begin = eoff[aedge];
        edge_end = eoff[aedge + 1];
        if (edge_begin < 0 || edge_begin >= num_ett ||
            edge_end < 0 || edge_end > num_ett) {
          P4EST_NOTICEF ("Invalid edge range %lld %d\n",
                         (long long) tree, edge);
          goto failure;
        }
        for (nett = edge_begin; nett < edge_end; ++nett) {
          ntree = ett[nett];
          nedge = (int) ete[nett] % P8EST_EDGES;
          if (tte[ntree * P8EST_EDGES + nedge] != aedge) {
            P4EST_NOTICEF ("Edge to edge reciprocity in %lld %d %lld\n",
                           (long long) tree, edge, (long long) nett);
            goto failure;
          }
          nflip = (int) ete[nett] / P8EST_EDGES;
          if (ntree == tree && nedge == edge) {
            if (flip != -1) {
              errcode = 1;
              break;
            }
            flip = nflip;
            continue;
          }
          ++errcount;
        }
        if (errcode > 0) {
          P4EST_NOTICEF ("Shared edge %lld %d %lld inconsistency %d\n",
                         (long long) tree, edge, (long long) nett, errcode);
          goto failure;
        }
        if (flip == -1 ||
            !(nflip1 == -1 || nflip1 == flip) ||
            !(nflip2 == -1 || nflip2 == flip)) {
          P4EST_NOTICEF ("Shared edge %lld %d inconsistent flip\n",
                         (long long) tree, edge);
          goto failure;
        }
      }
    }
#endif

    if (num_corners > 0) {
      for (corner = 0; corner < P4EST_CHILDREN; ++corner) {
        p4est_find_corner_transform (conn, tree, corner, &ci);
        acorner = ttc[tree * P4EST_CHILDREN + corner];
        if (acorner < -1 || acorner >= num_corners) {
          P4EST_NOTICEF ("Tree to corner out of range %lld %d\n",
                         (long long) tree, corner);
          goto failure;
        }
        if (acorner == -1) {
          continue;
        }
        errcode = errcount = 0;
        cfound = 0;
        corner_begin = coff[acorner];
        corner_end = coff[acorner + 1];
        if (corner_begin < 0 || corner_begin >= num_ctt ||
            corner_end < 0 || corner_end > num_ctt) {
          P4EST_NOTICEF ("Invalid corner range %lld %d\n",
                         (long long) tree, corner);
          goto failure;
        }
        for (nctt = corner_begin; nctt < corner_end; ++nctt) {
          ntree = ctt[nctt];
          ncorner = (int) ctc[nctt];
          if (ttc[ntree * P4EST_CHILDREN + ncorner] != acorner) {
            P4EST_NOTICEF ("Corner to corner reciprocity in %lld %d %lld\n",
                           (long long) tree, corner, (long long) nctt);
            goto failure;
          }
          if (ntree == tree && ncorner == corner) {
            if (cfound) {
              errcode = 1;
              break;
            }
            cfound = 1;
            continue;
          }
          ++errcount;
        }
        if (errcode > 0) {
          P4EST_NOTICEF ("Shared corner %lld %d %lld inconsistency %d\n",
                         (long long) tree, corner, (long long) nctt, errcode);
          goto failure;
        }
        if (!cfound) {
          P4EST_NOTICEF ("Shared corner %lld %d inconsistent count B\n",
                         (long long) tree, corner);
          goto failure;
        }
      }
    }
  }
  good = 1;

failure:
#ifdef P4_TO_P8
  sc_array_reset (eta);
#endif
  sc_array_reset (cta);

  return good;
}

int
p4est_connectivity_is_equal (p4est_connectivity_t * conn1,
                             p4est_connectivity_t * conn2)
{
  size_t              topsize, int8size;
  size_t              tcount;
  p4est_topidx_t      num_vertices;
#ifdef P4_TO_P8
  p4est_topidx_t      num_edges, num_ett;
#endif
  p4est_topidx_t      num_corners, num_ctt;

  topsize = sizeof (p4est_topidx_t);
  int8size = sizeof (int8_t);

  if (conn1->num_vertices != conn2->num_vertices ||
      conn1->num_trees != conn2->num_trees ||
#ifdef P4_TO_P8
      conn1->num_edges != conn2->num_edges ||
#endif
      conn1->num_corners != conn2->num_corners)
    return 0;

  num_vertices = conn1->num_vertices;
  if (num_vertices > 0) {
    P4EST_ASSERT (conn1->vertices != NULL && conn2->vertices != NULL);
    if (memcmp (conn1->vertices, conn2->vertices,
                sizeof (double) * 3 * num_vertices))
      return 0;

    P4EST_ASSERT (conn1->tree_to_vertex != NULL &&
                  conn2->tree_to_vertex != NULL);
    tcount = (size_t) (P4EST_CHILDREN * conn1->num_trees);
    if (memcmp (conn1->tree_to_vertex, conn2->tree_to_vertex,
                tcount * topsize))
      return 0;
  }

#ifdef P4_TO_P8
  tcount = (size_t) (P8EST_EDGES * conn1->num_trees);
  if (memcmp (conn1->tree_to_edge, conn2->tree_to_edge, tcount * topsize))
    return 0;
#endif

  tcount = (size_t) (P4EST_CHILDREN * conn1->num_trees);
  if (memcmp (conn1->tree_to_corner, conn2->tree_to_corner, tcount * topsize))
    return 0;

  tcount = (size_t) (P4EST_FACES * conn1->num_trees);
  if (memcmp (conn1->tree_to_tree, conn2->tree_to_tree, tcount * topsize) ||
      memcmp (conn1->tree_to_face, conn2->tree_to_face, tcount * int8size))
    return 0;

#ifdef P4_TO_P8
  num_edges = conn1->num_edges;
  num_ett = conn1->ett_offset[num_edges];
  if (memcmp (conn1->ett_offset, conn2->ett_offset,
              topsize * (num_edges + 1)) ||
      memcmp (conn1->edge_to_tree, conn2->edge_to_tree,
              topsize * num_ett) ||
      memcmp (conn1->edge_to_edge, conn2->edge_to_edge, int8size * num_ett))
    return 0;
#endif

  num_corners = conn1->num_corners;
  num_ctt = conn1->ctt_offset[num_corners];
  if (memcmp (conn1->ctt_offset, conn2->ctt_offset,
              topsize * (num_corners + 1)) ||
      memcmp (conn1->corner_to_tree, conn2->corner_to_tree,
              topsize * num_ctt) ||
      memcmp (conn1->corner_to_corner, conn2->corner_to_corner,
              int8size * num_ctt))
    return 0;

  return 1;
}

void
p4est_connectivity_save (const char *filename, p4est_connectivity_t * conn)
{
  int                 retval;
  char                magic6[6];
  size_t              u64z, topsize, int8size;
  size_t              tcount;
  uint64_t            array8[8];
  p4est_topidx_t      num_vertices, num_trees;
  p4est_topidx_t      num_edges, num_ett, num_corners, num_ctt;
  FILE               *file;

  P4EST_ASSERT (p4est_connectivity_is_valid (conn));

  file = fopen (filename, "wb");
  SC_CHECK_ABORT (file != NULL, "file open");

  num_vertices = conn->num_vertices;
  num_trees = conn->num_trees;
#ifdef P4_TO_P8
  num_edges = conn->num_edges;
  num_ett = conn->ett_offset[num_edges];
#else
  num_edges = num_ett = 0;
#endif
  num_corners = conn->num_corners;
  num_ctt = conn->ctt_offset[num_corners];

  strncpy (magic6, P4EST_STRING, 6);
  sc_fwrite (magic6, 6, 1, file, "write magic");

  u64z = sizeof (uint64_t);
  topsize = sizeof (p4est_topidx_t);
  int8size = sizeof (int8_t);
  array8[0] = P4EST_ONDISK_FORMAT;
  array8[1] = (uint64_t) topsize;
  array8[2] = (uint64_t) num_vertices;
  array8[3] = (uint64_t) num_trees;
  array8[4] = (uint64_t) num_edges;
  array8[5] = (uint64_t) num_ett;
  array8[6] = (uint64_t) num_corners;
  array8[7] = (uint64_t) num_ctt;
  sc_fwrite (array8, u64z, 8, file, "write header");

  if (num_vertices > 0)
    sc_fwrite (conn->vertices, sizeof (double), 3 * num_vertices, file,
               "write vertices");

#ifdef P4_TO_P8
  tcount = (size_t) (P8EST_EDGES * num_trees);
  if (num_edges > 0)
    sc_fwrite (conn->tree_to_edge, topsize, tcount, file, "write tte");
#endif

  tcount = (size_t) (P4EST_CHILDREN * num_trees);
  if (num_vertices > 0)
    sc_fwrite (conn->tree_to_vertex, topsize, tcount, file, "write ttv");
  if (num_corners > 0)
    sc_fwrite (conn->tree_to_corner, topsize, tcount, file, "write ttc");

  tcount = (size_t) (P4EST_FACES * num_trees);
  sc_fwrite (conn->tree_to_tree, topsize, tcount, file, "write ttt");
  sc_fwrite (conn->tree_to_face, int8size, tcount, file, "write ttf");

#ifdef P4_TO_P8
  sc_fwrite (conn->ett_offset, topsize, num_edges + 1, file,
             "write ett_offset");
  if (num_edges > 0) {
    sc_fwrite (conn->edge_to_tree, topsize, num_ett, file, "write ett");
    sc_fwrite (conn->edge_to_edge, int8size, num_ett, file, "write ete");
  }
#endif

  sc_fwrite (conn->ctt_offset, topsize, num_corners + 1, file,
             "write ctt_offset");
  if (num_corners > 0) {
    sc_fwrite (conn->corner_to_tree, topsize, num_ctt, file, "write ctt");
    sc_fwrite (conn->corner_to_corner, int8size, num_ctt, file, "write ctc");
  }

  retval = fclose (file);
  SC_CHECK_ABORT (retval == 0, "file close");
}

p4est_connectivity_t *
p4est_connectivity_load (const char *filename, long *length)
{
  int                 retval;
  char                magic6[6];
  size_t              u64z, topsize, int8size;
  size_t              tcount;
  uint64_t            array8[8];
  p4est_topidx_t      num_vertices, num_trees;
  p4est_topidx_t      num_edges, num_ett, num_corners, num_ctt;
  FILE               *file;
  p4est_connectivity_t *conn = NULL;

  file = fopen (filename, "rb");
  SC_CHECK_ABORT (file != NULL, "file open");

  sc_fread (magic6, 6, 1, file, "read magic");
  SC_CHECK_ABORT (strncmp (magic6, P4EST_STRING, 6) == 0, "invalid magic");

  u64z = sizeof (uint64_t);
  topsize = sizeof (p4est_topidx_t);
  int8size = sizeof (int8_t);
  sc_fread (array8, u64z, 8, file, "read header");

  SC_CHECK_ABORT (array8[0] == P4EST_ONDISK_FORMAT,
                  "on-disk format mismatch");
  SC_CHECK_ABORT (array8[1] == (uint64_t) topsize,
                  "p4est_topidx_t size mismatch");

  num_vertices = (p4est_topidx_t) array8[2];
  num_trees = (p4est_topidx_t) array8[3];
  num_edges = (p4est_topidx_t) array8[4];
  num_ett = (p4est_topidx_t) array8[5];
  num_corners = (p4est_topidx_t) array8[6];
  num_ctt = (p4est_topidx_t) array8[7];
  SC_CHECK_ABORT (num_vertices >= 0, "negative num_vertices");
  SC_CHECK_ABORT (num_trees >= 0, "negative num_trees");
#ifdef P4_TO_P8
  SC_CHECK_ABORT (num_edges >= 0, "negative num_edges");
  SC_CHECK_ABORT (num_ett >= 0, "negative num_ett");
#else
  SC_CHECK_ABORT (num_edges == 0, "num_edges must be zero in 2D");
  SC_CHECK_ABORT (num_ett == 0, "num_ett must be zero in 2D");
#endif
  SC_CHECK_ABORT (num_corners >= 0, "negative num_corners");
  SC_CHECK_ABORT (num_ctt >= 0, "negative num_ctt");

  conn = p4est_connectivity_new (num_vertices, num_trees,
#ifdef P4_TO_P8
                                 num_edges, num_ett,
#endif
                                 num_corners, num_ctt);

  if (num_vertices > 0)
    sc_fread (conn->vertices, sizeof (double), 3 * num_vertices, file,
              "read vertices");

#ifdef P4_TO_P8
  tcount = (size_t) (P8EST_EDGES * num_trees);
  if (num_edges > 0)
    sc_fread (conn->tree_to_edge, topsize, tcount, file, "read tte");
#endif

  tcount = (size_t) (P4EST_CHILDREN * num_trees);
  if (num_vertices > 0)
    sc_fread (conn->tree_to_vertex, topsize, tcount, file, "read ttv");
  if (num_corners > 0)
    sc_fread (conn->tree_to_corner, topsize, tcount, file, "read ttc");

  tcount = (size_t) (P4EST_FACES * num_trees);
  sc_fread (conn->tree_to_tree, topsize, tcount, file, "read ttt");
  sc_fread (conn->tree_to_face, int8size, tcount, file, "read ttf");

#ifdef P4_TO_P8
  sc_fread (conn->ett_offset, topsize, num_edges + 1, file,
            "read ett_offset");
  SC_CHECK_ABORT (num_ett == conn->ett_offset[num_edges], "num_ett mismatch");
  if (num_edges > 0) {
    sc_fread (conn->edge_to_tree, topsize, num_ett, file, "read ett");
    sc_fread (conn->edge_to_edge, int8size, num_ett, file, "read ete");
  }
#endif

  sc_fread (conn->ctt_offset, topsize, num_corners + 1, file,
            "read ctt_offset");
  SC_CHECK_ABORT (num_ctt == conn->ctt_offset[num_corners],
                  "num_ctt mismatch");
  if (num_corners > 0) {
    sc_fread (conn->corner_to_tree, topsize, num_ctt, file, "read ctt");
    sc_fread (conn->corner_to_corner, int8size, num_ctt, file, "read ctc");
  }

  if (length != NULL) {
    *length = ftell (file);
    SC_CHECK_ABORT (*length > 0, "file tell");
  }

  retval = fclose (file);
  SC_CHECK_ABORT (retval == 0, "file close");

  SC_CHECK_ABORT (p4est_connectivity_is_valid (conn), "invalid connectivity");

  return conn;
}

#ifndef P4_TO_P8

p4est_connectivity_t *
p4est_connectivity_new_unitsquare (void)
{
  const p4est_topidx_t num_vertices = 4;
  const p4est_topidx_t num_trees = 1;
  const p4est_topidx_t num_ctt = 0;
  const double        vertices[4 * 3] = {
    0, 0, 0,
    1, 0, 0,
    0, 1, 0,
    1, 1, 0,
  };
  const p4est_topidx_t tree_to_vertex[1 * 4] = {
    0, 1, 2, 3,
  };
  const p4est_topidx_t tree_to_tree[1 * 4] = {
    0, 0, 0, 0,
  };
  const int8_t        tree_to_face[1 * 4] = {
    0, 1, 2, 3,
  };

  return p4est_connectivity_new_copy (num_vertices, num_trees, 0,
                                      vertices, tree_to_vertex,
                                      tree_to_tree, tree_to_face,
                                      NULL, &num_ctt, NULL, NULL);
}

p4est_connectivity_t *
p4est_connectivity_new_periodic (void)
{
  const p4est_topidx_t num_vertices = 4;
  const p4est_topidx_t num_trees = 1;
  const p4est_topidx_t num_corners = 1;
  const double        vertices[4 * 3] = {
    0, 0, 0,
    1, 0, 0,
    0, 1, 0,
    1, 1, 0,
  };
  const p4est_topidx_t tree_to_vertex[1 * 4] = {
    0, 1, 2, 3,
  };
  const p4est_topidx_t tree_to_tree[1 * 4] = {
    0, 0, 0, 0,
  };
  const int8_t        tree_to_face[1 * 4] = {
    1, 0, 3, 2,
  };
  const p4est_topidx_t tree_to_corner[1 * 4] = {
    0, 0, 0, 0,
  };
  const p4est_topidx_t ctt_offset[1 + 1] = {
    0, 4,
  };
  const p4est_topidx_t corner_to_tree[4] = {
    0, 0, 0, 0,
  };
  const int8_t        corner_to_corner[4] = {
    0, 1, 2, 3,
  };

  return p4est_connectivity_new_copy (num_vertices, num_trees, num_corners,
                                      vertices, tree_to_vertex,
                                      tree_to_tree, tree_to_face,
                                      tree_to_corner, ctt_offset,
                                      corner_to_tree, corner_to_corner);
}

p4est_connectivity_t *
p4est_connectivity_new_rotwrap (void)
{
  const p4est_topidx_t num_vertices = 4;
  const p4est_topidx_t num_trees = 1;
  const p4est_topidx_t num_corners = 1;
  const double        vertices[4 * 3] = {
    0, 0, 0,
    1, 0, 0,
    0, 1, 0,
    1, 1, 0,
  };
  const p4est_topidx_t tree_to_vertex[1 * 4] = {
    0, 1, 2, 3,
  };
  const p4est_topidx_t tree_to_tree[1 * 4] = {
    0, 0, 0, 0,
  };
  const int8_t        tree_to_face[1 * 4] = {
    1, 0, 7, 6,
  };
  const p4est_topidx_t tree_to_corner[1 * 4] = {
    0, 0, 0, 0,
  };
  const p4est_topidx_t ctt_offset[1 + 1] = {
    0, 4,
  };
  const p4est_topidx_t corner_to_tree[4] = {
    0, 0, 0, 0,
  };
  const int8_t        corner_to_corner[4] = {
    0, 1, 2, 3,
  };

  return p4est_connectivity_new_copy (num_vertices, num_trees, num_corners,
                                      vertices, tree_to_vertex,
                                      tree_to_tree, tree_to_face,
                                      tree_to_corner, ctt_offset,
                                      corner_to_tree, corner_to_corner);
}

p4est_connectivity_t *
p4est_connectivity_new_corner (void)
{
  const p4est_topidx_t num_vertices = 7;
  const p4est_topidx_t num_trees = 3;
  const p4est_topidx_t num_corners = 1;
  const double        vertices[7 * 3] = {
    -1, -1, 0,
    0, -1, 0,
    0, 0, 1,
    1, 0, 1,
    1, 1, 1,
    0, 1, 1,
    -1, 0, 0,
  };
  const p4est_topidx_t tree_to_vertex[3 * 4] = {
    0, 1, 2, 3, 0, 2, 6, 5, 2, 3, 5, 4,
  };
  const p4est_topidx_t tree_to_tree[3 * 4] = {
    1, 0, 0, 2, 1, 2, 0, 1, 1, 2, 0, 2,
  };
  const int8_t        tree_to_face[3 * 4] = {
    2, 1, 2, 2, 0, 0, 0, 3, 1, 1, 3, 3,
  };
  const p4est_topidx_t tree_to_corner[3 * 4] = {
    -1, -1, 0, -1, -1, 0, -1, -1, 0, -1, -1, -1,
  };
  const p4est_topidx_t ctt_offset[1 + 1] = {
    0, 3,
  };
  const p4est_topidx_t corner_to_tree[3] = {
    0, 1, 2,
  };
  const int8_t        corner_to_corner[3] = {
    2, 1, 0,
  };

  return p4est_connectivity_new_copy (num_vertices, num_trees, num_corners,
                                      vertices, tree_to_vertex,
                                      tree_to_tree, tree_to_face,
                                      tree_to_corner, ctt_offset,
                                      corner_to_tree, corner_to_corner);
}

p4est_connectivity_t *
p4est_connectivity_new_moebius (void)
{
  const p4est_topidx_t num_vertices = 10;
  const p4est_topidx_t num_trees = 5;
  const p4est_topidx_t num_ctt = 0;
  const double        halfsqrt3 = .5 * sqrt (3.);
  const double        vertices[10 * 3] = {
    0, 0, 0,
    0, 1, 0,
    1, 0, 0,
    1, 1, 0,
    1.5, 0, halfsqrt3,
    1.5, 1, halfsqrt3,
    .5, .5, 1.5,
    .5, .5, 2,
    -.5, 0, halfsqrt3,
    -.5, 1, halfsqrt3,
  };
  const p4est_topidx_t tree_to_vertex[5 * 4] = {
    0, 2, 1, 3,
    3, 5, 2, 4,
    4, 6, 5, 7,
    6, 7, 9, 8,
    9, 8, 1, 0,
  };
  const p4est_topidx_t tree_to_tree[5 * 4] = {
    4, 1, 0, 0,
    0, 2, 1, 1,
    1, 3, 2, 2,
    3, 3, 2, 4,
    4, 4, 3, 0,
  };
  const int8_t        tree_to_face[5 * 4] = {
    7, 4, 2, 3,
    5, 4, 2, 3,
    5, 2, 2, 3,
    0, 1, 1, 2,
    0, 1, 3, 4,
  };

  return p4est_connectivity_new_copy (num_vertices, num_trees, 0,
                                      vertices, tree_to_vertex,
                                      tree_to_tree, tree_to_face,
                                      NULL, &num_ctt, NULL, NULL);
}

p4est_connectivity_t *
p4est_connectivity_new_star (void)
{
  const p4est_topidx_t num_vertices = 13;
  const p4est_topidx_t num_trees = 6;
  const p4est_topidx_t num_corners = 1;
  const p4est_topidx_t tree_to_vertex[6 * 4] = {
    0, 1, 3, 2,
    0, 3, 5, 4,
    5, 6, 0, 7,
    8, 7, 9, 0,
    9, 0, 10, 11,
    12, 1, 11, 0,
  };
  const p4est_topidx_t tree_to_tree[6 * 4] = {
    1, 0, 5, 0,
    2, 1, 0, 1,
    1, 2, 2, 3,
    3, 2, 3, 4,
    4, 5, 3, 4,
    5, 0, 5, 4,
  };
  const int8_t        tree_to_face[6 * 4] = {
    2, 1, 5, 3,
    4, 1, 0, 3,
    4, 1, 2, 5,
    0, 7, 2, 2,
    0, 7, 3, 3,
    0, 6, 2, 5,
  };
  const p4est_topidx_t tree_to_corner[6 * 4] = {
    0, -1, -1, -1, 0, -1, -1, -1, -1, -1, 0, -1,
    -1, -1, -1, 0, -1, 0, -1, -1, -1, -1, -1, 0,
  };
  const p4est_topidx_t ctt_offset[1 + 1] = {
    0, 6,
  };
  const p4est_topidx_t corner_to_tree[6] = {
    0, 1, 2, 3, 4, 5,
  };
  const int8_t        corner_to_corner[6] = {
    0, 0, 2, 3, 1, 3,
  };
  const double        pi = 4.0 * atan (1.0);
  const double        r1 = 1.;
  const double        r2 = 1.5;
  double              vertices[13 * 3];
  int                 i;

  vertices[0 * 3 + 0] = 0;
  vertices[0 * 3 + 1] = 0;
  vertices[0 * 3 + 2] = 0;
  for (i = 0; i < 6; ++i) {
    vertices[(2 * i + 1) * 3 + 0] = r1 * cos (i * pi / 3);
    vertices[(2 * i + 1) * 3 + 1] = r1 * sin (i * pi / 3);
    vertices[(2 * i + 1) * 3 + 2] = 0;
    vertices[(2 * i + 2) * 3 + 0] = r2 * cos ((i + .5) * pi / 3);
    vertices[(2 * i + 2) * 3 + 1] = r2 * sin ((i + .5) * pi / 3);
    vertices[(2 * i + 2) * 3 + 2] = 0;
  }

  return p4est_connectivity_new_copy (num_vertices, num_trees, num_corners,
                                      vertices, tree_to_vertex,
                                      tree_to_tree, tree_to_face,
                                      tree_to_corner, ctt_offset,
                                      corner_to_tree, corner_to_corner);
}
#endif /* !P4_TO_P8 */

static inline void
brick_linear_to_xyz (p4est_topidx_t ti, const int logx[P4EST_DIM],
                     const int rankx[P4EST_DIM], p4est_topidx_t tx[P4EST_DIM])
{
  int                 i, j, k;
  int                 lastlog = 0;

  for (i = 0; i < P4EST_DIM; i++) {
    tx[i] = 0;
  }

  for (i = 0; i < P4EST_DIM - 1; i++) {
    p4est_topidx_t      tempx[3] = { 0, 0, 0 };
    int                 logi = logx[rankx[i]] - lastlog;
    int                 idx[3] = { -1, -1, -1 };
    int                 c = 0;

    for (k = 0; k < P4EST_DIM - i; k++) {
      int                 d = rankx[i + k];

      idx[d] = 0;
    }
    for (k = 0; k < P4EST_DIM; k++) {
      if (idx[k] == 0) {
        idx[k] = c++;
      }
    }

    for (j = 0; j < logi; j++) {
      int                 base = (P4EST_DIM - i) * j;
      int                 shift = (P4EST_DIM - i - 1) * j;

      for (k = 0; k < P4EST_DIM; k++) {
        int                 id = idx[k];

        if (id >= 0) {
          tempx[k] |= (ti & (1 << (base + id))) >> (shift + id);
        }
      }
    }
    for (k = 0; k < P4EST_DIM; k++) {
      tx[k] += (tempx[k] << lastlog);
    }
    lastlog += logi;
    ti >>= (P4EST_DIM - i) * logi;
  }
  tx[rankx[P4EST_DIM - 1]] += (ti << lastlog);
}

static inline       p4est_topidx_t
brick_xyz_to_linear (const p4est_topidx_t tx[P4EST_DIM],
                     const int logx[P4EST_DIM], const int rankx[P4EST_DIM])
{
  int                 i, j, k;
  int                 lastlog = logx[rankx[P4EST_DIM - 2]];
  p4est_topidx_t      ti = tx[rankx[P4EST_DIM - 1]] >> lastlog;

  for (i = P4EST_DIM - 2; i >= 0; i--) {
    p4est_topidx_t      tempx[3] = { 0, 0, 0 };
    int                 logi =
      (i == 0) ? lastlog : lastlog - logx[rankx[i - 1]];
    int                 idx[3] = { -1, -1, -1 };
    int                 c = 0;

    for (k = 0; k < P4EST_DIM - i; k++) {
      int                 d = rankx[i + k];

      idx[d] = 0;
    }
    for (k = 0; k < P4EST_DIM; k++) {
      if (idx[k] == 0) {
        idx[k] = c++;
      }
    }

    ti <<= (P4EST_DIM - i) * logi;
    lastlog -= logi;
    for (k = 0; k < P4EST_DIM; k++) {
      tempx[k] = tx[k] >> lastlog;
    }
    for (j = 0; j < logi; j++) {
      int                 shift = (P4EST_DIM - i - 1) * j;

      for (k = 0; k < P4EST_DIM; k++) {
        int                 id = idx[k];

        if (id >= 0) {
          ti |= (tempx[k] & (1 << j)) << (shift + id);
        }
      }
    }
  }

  return ti;
}

p4est_connectivity_t *
#ifndef P4_TO_P8
p4est_connectivity_new_brick (int mi, int ni, int periodic_a, int periodic_b)
#else
p8est_connectivity_new_brick (int mi, int ni, int pi, int periodic_a,
                              int periodic_b, int periodic_c)
#endif
{
#ifndef P4_TO_P8
  const p4est_topidx_t m = (p4est_topidx_t) mi;
  const p4est_topidx_t n = (p4est_topidx_t) ni;
  const p4est_topidx_t mc = periodic_a ? m : (m - 1);
  const p4est_topidx_t nc = periodic_b ? n : (n - 1);
  const p4est_topidx_t num_trees = m * n;
  const p4est_topidx_t num_corners = mc * nc;
  const p4est_topidx_t num_ctt = P4EST_CHILDREN * num_corners;
  const p4est_topidx_t num_vertices = (m + 1) * (n + 1);
  const int           periodic[P4EST_DIM] = { periodic_a, periodic_b };
  const p4est_topidx_t max[P4EST_DIM] = { m - 1, n - 1 };
#else
  const p4est_topidx_t m = (p4est_topidx_t) mi;
  const p4est_topidx_t n = (p4est_topidx_t) ni;
  const p4est_topidx_t p = (p4est_topidx_t) pi;
  const p4est_topidx_t mc = periodic_a ? m : (m - 1);
  const p4est_topidx_t nc = periodic_b ? n : (n - 1);
  const p4est_topidx_t pc = periodic_c ? p : (p - 1);
  const p4est_topidx_t num_trees = m * n * p;
  const p4est_topidx_t num_corners = mc * nc * pc;
  const p4est_topidx_t num_ctt = P4EST_CHILDREN * num_corners;
  const p4est_topidx_t num_edges = m * nc * pc + mc * n * pc + mc * nc * p;
  const p4est_topidx_t num_ett = 4 * num_edges;
  const p4est_topidx_t num_vertices = (m + 1) * (n + 1) * (p + 1);
  const int           periodic[P4EST_DIM] = { periodic_a, periodic_b,
    periodic_c
  };
  const p4est_topidx_t max[P4EST_DIM] = { m - 1, n - 1, p - 1 };
#endif
  double             *vertices;
  p4est_topidx_t     *tree_to_vertex;
  p4est_topidx_t     *tree_to_tree;
  int8_t             *tree_to_face;
  p4est_topidx_t     *tree_to_corner;
  p4est_topidx_t     *ctt_offset;
  p4est_topidx_t     *corner_to_tree;
  int8_t             *corner_to_corner;
  p4est_topidx_t      n_iter;
  int                 logx[P4EST_DIM];
  int                 rankx[P4EST_DIM];
  int                 i, j, l;
  p4est_topidx_t      ti, tj, tk;
  p4est_topidx_t      tx, ty;
  p4est_topidx_t      tf[P4EST_FACES], tc[P4EST_CHILDREN];
  p4est_topidx_t      coord[P4EST_DIM], coord2[P4EST_DIM], ttemp;
  p4est_topidx_t     *linear_to_tree;
  p4est_topidx_t     *tree_to_corner2;
  p4est_topidx_t      vcount = 0, vicount = 0;
  int                 c[P4EST_DIM];
  p4est_connectivity_t *conn;
#ifdef P4_TO_P8
  p4est_topidx_t      tl;
  p4est_topidx_t      tz;
  p4est_topidx_t      te[P8EST_EDGES];
  p4est_topidx_t     *tree_to_edge;
  p4est_topidx_t     *ett_offset;
  p4est_topidx_t     *edge_to_tree;
  int8_t             *edge_to_edge;
  p4est_topidx_t     *tree_to_edge2;
  int                 dir1, dir2;
#endif

#ifndef P4_TO_P8
  P4EST_ASSERT (m > 0 && n > 0);
#else
  P4EST_ASSERT (m > 0 && n > 0 && p > 0);
#endif

  conn = p4est_connectivity_new (num_vertices, num_trees,
#ifdef P4_TO_P8
                                 num_edges, num_ett,
#endif
                                 num_corners, num_ctt);

  vertices = conn->vertices;
  tree_to_vertex = conn->tree_to_vertex;
  tree_to_tree = conn->tree_to_tree;
  tree_to_face = conn->tree_to_face;
#ifdef P4_TO_P8
  tree_to_edge = conn->tree_to_edge;
  ett_offset = conn->ett_offset;
  edge_to_tree = conn->edge_to_tree;
  edge_to_edge = conn->edge_to_edge;
#endif
  tree_to_corner = conn->tree_to_corner;
  ctt_offset = conn->ctt_offset;
  corner_to_tree = conn->corner_to_tree;
  corner_to_corner = conn->corner_to_corner;

#ifdef P4_TO_P8
  for (ti = 0; ti < num_edges + 1; ti++) {
    ett_offset[ti] = 4 * ti;
  }
#endif

  for (ti = 0; ti < num_corners + 1; ti++) {
    ctt_offset[ti] = P4EST_CHILDREN * ti;
  }

  for (ti = 0; ti < P4EST_CHILDREN * num_trees; ti++) {
    tree_to_vertex[ti] = -1;
  }

  logx[0] = SC_LOG2_32 (m - 1) + 1;
  logx[1] = SC_LOG2_32 (n - 1) + 1;
  n_iter = (1 << logx[0]) * (1 << logx[1]);
  if (logx[0] <= logx[1]) {
    rankx[0] = 0;
    rankx[1] = 1;
  }
  else {
    rankx[0] = 1;
    rankx[1] = 0;
  }
#ifdef P4_TO_P8
  logx[2] = SC_LOG2_32 (p - 1) + 1;
  n_iter *= (1 << logx[2]);
  if (logx[2] < logx[rankx[0]]) {
    rankx[2] = rankx[1];
    rankx[1] = rankx[0];
    rankx[0] = 2;
  }
  else if (logx[rankx[1]] <= logx[2]) {
    rankx[2] = 2;
  }
  else {
    rankx[2] = rankx[1];
    rankx[1] = 2;
  }
#endif

  linear_to_tree = P4EST_ALLOC (p4est_topidx_t, n_iter);
  tree_to_corner2 = P4EST_ALLOC (p4est_topidx_t, num_trees);
#ifdef P4_TO_P8
  tree_to_edge2 = P4EST_ALLOC (p4est_topidx_t, 3 * num_trees);
#endif

  tj = 0;
  tk = 0;
#ifdef P4_TO_P8
  tl = 0;
#endif
  for (ti = 0; ti < n_iter; ti++) {
    brick_linear_to_xyz (ti, logx, rankx, coord);
    tx = coord[0];
    ty = coord[1];
#ifdef P4_TO_P8
    tz = coord[2];
#endif
    if (tx < m && ty < n &&
#ifdef P4_TO_P8
        tz < p &&
#endif
        1) {
      linear_to_tree[ti] = tj;
      if ((tx < m - 1 || periodic_a) && (ty < n - 1 || periodic_b) &&
#ifdef P4_TO_P8
          (tz < p - 1 || periodic_c) &&
#endif
          1) {
        tree_to_corner2[tj] = tk++;
#ifdef P4_TO_P8
        tree_to_edge2[3 * tj] = tl++;
        tree_to_edge2[3 * tj + 1] = tl++;
        tree_to_edge2[3 * tj + 2] = tl++;
#endif
      }
      else {
        tree_to_corner2[tj] = -1;
#ifdef P4_TO_P8
        if ((ty < n - 1 || periodic_b) && (tz < p - 1 || periodic_c)) {
          tree_to_edge2[3 * tj] = tl++;
        }
        else {
          tree_to_edge2[3 * tj] = -1;
        }
        if ((tx < m - 1 || periodic_a) && (tz < p - 1 || periodic_c)) {
          tree_to_edge2[3 * tj + 1] = tl++;
        }
        else {
          tree_to_edge2[3 * tj + 1] = -1;
        }
        if ((tx < m - 1 || periodic_a) && (ty < n - 1 || periodic_b)) {
          tree_to_edge2[3 * tj + 2] = tl++;
        }
        else {
          tree_to_edge2[3 * tj + 2] = -1;
        }
#endif
      }
      tj++;
    }
    else {
      linear_to_tree[ti] = -1;
    }
  }
  P4EST_ASSERT (tj == num_trees);
  P4EST_ASSERT (tk == num_corners);
#ifdef P4_TO_P8
  P4EST_ASSERT (tl == num_edges);
#endif

  for (ti = 0; ti < n_iter; ti++) {
    brick_linear_to_xyz (ti, logx, rankx, coord);
    tx = coord[0];
    ty = coord[1];
#ifdef P4_TO_P8
    tz = coord[2];
#endif
    if (tx < m && ty < n &&
#ifdef P4_TO_P8
        tz < p &&
#endif
        1) {
      tj = linear_to_tree[ti];
      P4EST_ASSERT (tj >= 0);
      for (i = 0; i < P4EST_DIM; i++) {
        for (j = 0; j < 2; j++) {
          l = 2 * i + j;
          coord2[0] = ((tx + ((i == 0) ? (2 * j - 1) : 0)) + m) % m;
          coord2[1] = ((ty + ((i == 1) ? (2 * j - 1) : 0)) + n) % n;
#ifdef P4_TO_P8
          coord2[2] = ((tz + ((i == 2) ? (2 * j - 1) : 0)) + p) % p;
#endif
          tf[l] = brick_xyz_to_linear (coord2, logx, rankx);
          P4EST_ASSERT (tf[l] < n_iter);
          tf[l] = linear_to_tree[tf[l]];
          P4EST_ASSERT (tf[l] >= 0);
        }
#ifdef P4_TO_P8
        for (j = 0; j < 4; j++) {
          l = 4 * i + j;
          coord2[0] = ((tx + ((i == 0) ? 0 : (2 * (j & 1) - 1))) + m) % m;
          coord2[1] = ((ty + ((i == 1) ? 0 :
                              (2 * ((i == 0) ? (j & 1) : (j / 2)) - 1))) +
                       n) % n;
          coord2[2] = ((tz + ((i == 2) ? 0 : (2 * (j / 2) - 1))) + p) % p;
          te[l] = brick_xyz_to_linear (coord2, logx, rankx);
          P4EST_ASSERT (te[l] < n_iter);
          te[l] = linear_to_tree[te[l]];
          P4EST_ASSERT (te[l] >= 0);
        }
#endif
      }
      for (i = 0; i < P4EST_CHILDREN; i++) {
        coord2[0] = ((tx + (((i & 1) == 0) ? -1 : 1)) + m) % m;
        coord2[1] = ((ty + ((((i >> 1) & 1) == 0) ? -1 : 1)) + n) % n;
#ifdef P4_TO_P8
        coord2[2] = ((tz + (((i >> 2) == 0) ? -1 : 1)) + p) % p;
#endif
        tc[i] = brick_xyz_to_linear (coord2, logx, rankx);
        P4EST_ASSERT (tc[i] < n_iter);
        tc[i] = linear_to_tree[tc[i]];
        P4EST_ASSERT (tc[i] >= 0);
      }
      for (i = 0; i < P4EST_DIM; i++) {
        for (j = 0; j < 2; j++) {
          l = i * 2 + j;
          if (!periodic[i] &&
              ((coord[i] == 0 && j == 0) || (coord[i] == max[i] && j == 1))) {
            tree_to_tree[tj * P4EST_FACES + l] = tj;
            tree_to_face[tj * P4EST_FACES + l] = (int8_t) l;
          }
          else {
            tree_to_tree[tj * P4EST_FACES + l] = tf[l];
            tree_to_face[tj * P4EST_FACES + l] = (int8_t) (i * 2 + (j ^ 1));
          }
        }
#ifdef P4_TO_P8
        if (tree_to_edge != NULL) {
          /** dir1, dir2 should be in correct z order */
          dir1 = (i == 0) ? 1 : 0;
          dir2 = (i == 2) ? 1 : 2;
          for (j = 0; j < 4; j++) {
            l = i * 4 + j;
            if ((!periodic[dir1] &&
                 ((coord[dir1] == 0 && (j & 1) == 0) ||
                  (coord[dir1] == max[dir1] && (j & 1) == 1))) ||
                (!periodic[dir2] &&
                 ((coord[dir2] == 0 && (j / 2) == 0) ||
                  (coord[dir2] == max[dir2] && (j / 2) == 1)))) {
              tree_to_edge[tj * P8EST_EDGES + l] = -1;
            }
            else {
              switch (j) {
              case 0:
                ttemp = tree_to_edge2[te[l] * 3 + i];
                break;
              case 1:
                ttemp = tree_to_edge2[tf[dir2 * 2] * 3 + i];
                break;
              case 2:
                ttemp = tree_to_edge2[tf[dir1 * 2] * 3 + i];
                break;
              case 3:
                ttemp = tree_to_edge2[tj * 3 + i];
                break;
              default:
                SC_ABORT_NOT_REACHED ();
              }
              P4EST_ASSERT (ttemp >= 0);
              tree_to_edge[tj * P8EST_EDGES + l] = ttemp;
              edge_to_tree[4 * ttemp + (3 - j)] = tj;
              edge_to_edge[4 * ttemp + (3 - j)] = (int8_t) l;
            }
          }
        }
#endif
      }
      for (i = 0; i < P4EST_CHILDREN; i++) {
        if (tree_to_corner != NULL) {
          c[0] = i & 1;
          c[1] = (i >> 1) & 1;
#ifdef P4_TO_P8
          c[2] = i >> 2;
#endif
          if ((!periodic[0] &&
               ((coord[0] == 0 && c[0] == 0) ||
                (coord[0] == max[0] && c[0] == 1))) ||
              (!periodic[1] &&
               ((coord[1] == 0 && c[1] == 0) ||
                (coord[1] == max[1] && c[1] == 1))) ||
#ifdef P4_TO_P8
              (!periodic[2] &&
               ((coord[2] == 0 && c[2] == 0) ||
                (coord[2] == max[2] && c[2] == 1))) ||
#endif
              0) {
            tree_to_corner[tj * P4EST_CHILDREN + i] = -1;
          }
          else {
            switch (i) {
#ifndef P4_TO_P8
            case 0:
              ttemp = tc[0];
              break;
            case 1:
              ttemp = tf[2];
              break;
            case 2:
              ttemp = tf[0];
              break;
            case 3:
              ttemp = tj;
              break;
#else
            case 0:
              ttemp = tc[0];
              break;
            case 1:
              ttemp = te[0];
              break;
            case 2:
              ttemp = te[4];
              break;
            case 3:
              ttemp = tf[4];
              break;
            case 4:
              ttemp = te[8];
              break;
            case 5:
              ttemp = tf[2];
              break;
            case 6:
              ttemp = tf[0];
              break;
            case 7:
              ttemp = tj;
              break;
#endif
            default:
              SC_ABORT_NOT_REACHED ();
            }
            ttemp = tree_to_corner2[ttemp];
            P4EST_ASSERT (ttemp >= 0);
            tree_to_corner[tj * P4EST_CHILDREN + i] = ttemp;
            corner_to_tree[ttemp * P4EST_CHILDREN +
                           (P4EST_CHILDREN - 1 - i)] = tj;
            corner_to_corner[ttemp * P4EST_CHILDREN +
                             (P4EST_CHILDREN - 1 - i)] = (int8_t) i;
          }
        }
#ifdef P4_TO_P8
        if (tz > 0 && (i >> 2) == 0) {
          tree_to_vertex[tj * P4EST_CHILDREN + i] =
            tree_to_vertex[tf[4] * P4EST_CHILDREN + i + 4];
        }
        else
#endif
        if (ty > 0 && ((i >> 1) & 1) == 0) {
          tree_to_vertex[tj * P4EST_CHILDREN + i] =
            tree_to_vertex[tf[2] * P4EST_CHILDREN + i + 2];
        }
        else if (tx > 0 && (i & 1) == 0) {
          tree_to_vertex[tj * P4EST_CHILDREN + i] =
            tree_to_vertex[tf[0] * P4EST_CHILDREN + i + 1];
        }
        else {
          tree_to_vertex[tj * P4EST_CHILDREN + i] = vcount++;
          vertices[vicount++] = (double) (tx + (i & 1));
          vertices[vicount++] = (double) (ty + ((i >> 1) & 1));
#ifndef P4_TO_P8
          vertices[vicount++] = 0.;
#else
          vertices[vicount++] = (double) (tz + (i >> 2));
#endif
        }
      }
    }
  }

  P4EST_ASSERT (vcount == num_vertices);

  P4EST_FREE (linear_to_tree);
  P4EST_FREE (tree_to_corner2);
#ifdef P4_TO_P8
  P4EST_FREE (tree_to_edge2);
#endif

  P4EST_ASSERT (p4est_connectivity_is_valid (conn));

  return conn;
}

p4est_topidx_t
p4est_find_face_transform (p4est_connectivity_t * connectivity,
                           p4est_topidx_t itree, int iface, int ftransform[])
{
  int                 target_code, target_face, orientation;
#ifdef P4_TO_P8
  int                 reverse;
#ifdef P4EST_DEBUG
  int                 i;
  int                *my_axis = &ftransform[0];
  int                *target_axis = &ftransform[3];
#endif
#endif
  p4est_topidx_t      target_tree;

  P4EST_ASSERT (itree >= 0 && itree < connectivity->num_trees);
  P4EST_ASSERT (iface >= 0 && iface < P4EST_FACES);

  target_tree = connectivity->tree_to_tree[P4EST_FACES * itree + iface];
  target_code = (int) connectivity->tree_to_face[P4EST_FACES * itree + iface];
  target_face = target_code % P4EST_FACES;
  orientation = target_code / P4EST_FACES;

  P4EST_ASSERT (0 <= target_face && target_face < P4EST_FACES);
  P4EST_ASSERT (0 <= orientation && orientation < P4EST_HALF);

  if (target_tree == itree && target_face == iface) {
    P4EST_ASSERT (orientation == 0);
    return -1;
  }

#ifdef P4_TO_P8
  /* the code that was here before is now in test/test_face_transform3.c */
  ftransform[0] = iface < 2 ? 1 : 0;
  ftransform[1] = iface < 4 ? 2 : 1;
  ftransform[2] = iface / 2;
  reverse =
    p8est_face_permutation_refs[0][iface] ^
    p8est_face_permutation_refs[0][target_face] ^
    (orientation == 0 || orientation == 3);
  ftransform[3 + reverse] = target_face < 2 ? 1 : 0;
  ftransform[3 + !reverse] = target_face < 4 ? 2 : 1;
  ftransform[5] = target_face / 2;
  reverse = (p8est_face_permutation_refs[iface][target_face] == 1);
  ftransform[6 + reverse] = (orientation & 1);
  ftransform[6 + !reverse] = (orientation >> 1);
  ftransform[8] = 2 * (iface & 1) + (target_face & 1);

#ifdef P4EST_DEBUG
  for (i = 0; i < 3; ++i) {
    P4EST_ASSERT (0 <= my_axis[i] && my_axis[i] < 3);
    P4EST_ASSERT (0 <= target_axis[i] && target_axis[i] < 3);
  }
  P4EST_ASSERT (my_axis[0] != my_axis[1] &&
                my_axis[0] != my_axis[2] && my_axis[1] != my_axis[2]);
  P4EST_ASSERT (target_axis[0] != target_axis[1] &&
                target_axis[0] != target_axis[2] &&
                target_axis[1] != target_axis[2]);
#endif
#else
  ftransform[2] = iface / 2;
  ftransform[1] = 0;
  ftransform[0] = 1 - ftransform[2];
  ftransform[5] = target_face / 2;
  ftransform[4] = 0;
  ftransform[3] = 1 - ftransform[5];
  ftransform[6] = orientation;
  ftransform[7] = 0;
  ftransform[8] = 2 * (iface & 1) + (target_face & 1);
#endif

  return target_tree;
}

void
p4est_find_corner_transform (p4est_connectivity_t * conn,
                             p4est_topidx_t itree, int icorner,
                             p4est_corner_info_t * ci)
{
  int                 i;
  int                 iface[P4EST_DIM], nface[P4EST_DIM];
  int                 orient[P4EST_DIM], fcorner[P4EST_DIM];
  int                 ncorner, ncode, fc, nc;
  int                 omit;
#ifdef P4EST_DEBUG
  size_t              expected_count;
#endif
  p4est_topidx_t      corner_trees, ctree, nctree;
  p4est_topidx_t      acorner, ntree[P4EST_DIM];
#ifdef P4_TO_P8
  int                 edge_ignored;
  int                 iedge[3], iwhich[3];
  int                 pref, pset;
  size_t              jz;
  p4est_topidx_t      aedge[3];
  p8est_edge_info_t   ei[3];
  sc_array_t         *eta[3];
  p8est_edge_transform_t *et;
#endif
  sc_array_t         *cta = &ci->corner_transforms;
  p4est_corner_transform_t *ct;

  P4EST_ASSERT (0 <= itree && itree < conn->num_trees);
  P4EST_ASSERT (0 <= icorner && icorner < P4EST_CHILDREN);
  P4EST_ASSERT (cta->elem_size == sizeof (p4est_corner_transform_t));

  ci->icorner = (int8_t) icorner;
  sc_array_resize (cta, 0);
  if (conn->num_corners == 0) {
    return;
  }
  acorner = conn->tree_to_corner[P4EST_CHILDREN * itree + icorner];
  if (acorner == -1) {
    return;
  }

  /* find the face neighbors */
  for (i = 0; i < P4EST_DIM; ++i) {
    iface[i] = p4est_corner_faces[icorner][i];
    ntree[i] = conn->tree_to_tree[P4EST_FACES * itree + iface[i]];
    ncode = (int) conn->tree_to_face[P4EST_FACES * itree + iface[i]];
    if (ntree[i] == itree && ncode == iface[i]) {       /* domain boundary */
      ntree[i] = -1;
      nface[i] = -1;
      orient[i] = -1;
      fcorner[i] = -1;
    }
    else {
      nface[i] = ncode % P4EST_FACES;
      orient[i] = ncode / P4EST_FACES;
      fcorner[i] = p4est_corner_face_corners[icorner][iface[i]];
      P4EST_ASSERT (fcorner[i] >= 0);
    }
  }
  corner_trees =                /* same type */
    conn->ctt_offset[acorner + 1] - conn->ctt_offset[acorner];

#ifdef P4_TO_P8
  /* find the three edge transforms */
  if (conn->num_edges == 0) {
    eta[0] = eta[1] = eta[2] = NULL;
    aedge[0] = aedge[1] = aedge[2] = -1;
  }
  else {
    for (i = 0; i < 3; ++i) {
      iedge[i] = p8est_corner_edges[icorner][i];
      aedge[i] = conn->tree_to_edge[P8EST_EDGES * itree + iedge[i]];
      if (aedge[i] == -1) {
        eta[i] = NULL;
        continue;
      }
      iwhich[i] = (p8est_edge_corners[iedge[i]][1] == icorner);
      P4EST_ASSERT (p8est_edge_corners[iedge[i]][iwhich[i]] == icorner);

      eta[i] = &ei[i].edge_transforms;
      sc_array_init (eta[i], sizeof (p8est_edge_transform_t));
      p8est_find_edge_transform (conn, itree, iedge[i], &ei[i]);
    }
  }
  edge_ignored = 0;
#endif

  /* collect all corners that are not from face or edge neighbors */
  for (ctree = 0; ctree < corner_trees; ++ctree) {
    nctree = conn->corner_to_tree[conn->ctt_offset[acorner] + ctree];
    ncorner = (int) conn->corner_to_corner[conn->ctt_offset[acorner] + ctree];
    if (ncorner == icorner && nctree == itree) {
      continue;
    }

    /* rule out face neighbors */
    omit = 0;
    for (i = 0; i < P4EST_DIM; ++i) {
      if (nctree == ntree[i]) {
        P4EST_ASSERT (fcorner[i] >= 0);
#ifdef P4_TO_P8
        pref = p8est_face_permutation_refs[iface[i]][nface[i]];
        pset = p8est_face_permutation_sets[pref][orient[i]];
        fc = p8est_face_permutations[pset][fcorner[i]];
#else
        fc = fcorner[i] ^ orient[i];
#endif
        nc = p4est_face_corners[nface[i]][fc];

        if (nc == ncorner) {
          omit = 1;
          break;
        }
      }
    }
    if (omit)
      continue;

#ifdef P4_TO_P8
    /* rule out edge neighbors */
    omit = 0;
    for (i = 0; i < 3; ++i) {
      if (aedge[i] == -1) {
        continue;
      }
      for (jz = 0; jz < eta[i]->elem_count; ++jz) {
        et = p8est_edge_array_index (eta[i], jz);
        if (nctree == et->ntree) {
          nc = p8est_edge_corners[et->nedge][et->nflip ^ iwhich[i]];

          if (nc == ncorner) {
            omit = 1;
            break;
          }
        }
      }
      if (omit)
        break;
    }
    if (omit) {
      ++edge_ignored;
      continue;
    }
#endif

    /* else we have a true all-diagonal corner with ntree */
    ct = (p4est_corner_transform_t *) sc_array_push (cta);
    ct->ntree = nctree;
    ct->ncorner = (int8_t) ncorner;
  }
#ifdef P4EST_DEBUG
  expected_count = cta->elem_count + 1 + (ntree[0] != -1) + (ntree[1] != -1);
#ifdef P4_TO_P8
  expected_count += (ntree[2] != -1) + edge_ignored;
#endif
  P4EST_ASSERT ((size_t) corner_trees == expected_count);
#endif

#ifdef P4_TO_P8
  for (i = 0; i < 3; ++i) {
    if (aedge[i] >= 0) {
      sc_array_reset (eta[i]);
    }
  }
#endif
}

#ifdef P4EST_METIS

static int
reorder_comp (const void *a, const void *b)
{
  const int          *A = (const int *) a;
  const int          *B = (const int *) b;

  if (A[0] < B[0]) {
    return -1;
  }
  else if (B[0] < A[0]) {
    return 1;
  }
  else {
    return (a - b);
  }
}

void
p4est_connectivity_reorder (p4est_connectivity_t * conn, int k, MPI_Comm comm,
                            int conntype)
{
  int                 n = (int) conn->num_trees;
  int                *xadj;
  int                *adjncy;
  int                *part;
  int                 totaldeg;
  int                 degree;
  int                 i, j, l;
  int                 rank = -1;
  p4est_corner_info_t ci;
  sc_array_t         *cta = &ci.corner_transforms;
  p4est_corner_transform_t *ct;
#ifdef P4_TO_P8
  p8est_edge_info_t   ei;
  sc_array_t         *eta = &ei.edge_transforms;
  p8est_edge_transform_t *et;
#endif
  int                 wgtflag = 0;      /* do not use weights */
  int                 numflag = 0;      /* C-style numbering */
  int                 options[5] = { 0, 1, 1, 1, 1 };   /* default options */
  int                 volume = -1;
  size_t              zz;
  int                 mpiret = MPI_Comm_rank (comm, &rank);
  sc_array_t         *newid;
  size_t             *zp;
  sc_array_t          array_view;
  sc_array_t         *sorter;
  int                *ip;
  int                 count;

  SC_CHECK_MPI (mpiret);

  /* part will hold the partition number of each tree */
  part = P4EST_ALLOC (int, n);

  if (!rank) {

    xadj = P4EST_ALLOC (int, n + 1);

    switch (conntype) {
    case 0:
      degree = P4EST_FACES;
      break;
    case (P4EST_DIM - 1):
      sc_array_init (cta, sizeof (p4est_corner_transform_t));
#ifndef P4_TO_P8
      degree = 8;
#else
      degree = 26;
      sc_array_init (eta, sizeof (p8est_edge_transform_t));
#endif
      break;
#ifdef P4_TO_P8
    case 1:
      sc_array_init (eta, sizeof (p8est_edge_transform_t));
      degree = 18;
      break;
#endif
    default:
      SC_ABORT_NOT_REACHED ();
    }

    if (degree == P4EST_FACES) {
      /* each tree has the same: metis shouldn't have any trouble with a
       * loop on a face/edge corner that has no neighbor */
      for (i = 0; i < n + 1; i++) {
        xadj[i] = P4EST_FACES * i;
      }
      adjncy = P4EST_ALLOC (int, P4EST_FACES * n);
      for (i = 0; i < n; i++) {
        for (j = 0; j < P4EST_FACES; j++) {
          adjncy[P4EST_FACES * i + j] =
            conn->tree_to_tree[P4EST_FACES * i + j];
        }
      }
    }
    else {
      totaldeg = 0;
      xadj[0] = 0;
      for (i = 0; i < n; i++) {
        totaldeg += P4EST_FACES;
        if (conntype == (P4EST_DIM - 1)) {
          for (j = 0; j < P4EST_CHILDREN; j++) {
            /* add the number of strict corner neighbors */
            p4est_find_corner_transform (conn, (p4est_topidx_t) i, j, &ci);
            totaldeg += (int) cta->elem_count;
          }
        }
#ifdef P4_TO_P8
        if (conntype >= 1) {
          /* add the number of strict edge neighbors */
          for (j = 0; j < P8EST_EDGES; j++) {
            p8est_find_edge_transform (conn, (p4est_topidx_t) i, j, &ei);
            totaldeg += (int) eta->elem_count;
          }
        }
#endif
        xadj[i + 1] = totaldeg;
      }

      adjncy = P4EST_ALLOC (int, totaldeg);

      l = 0;
      for (i = 0; i < n; i++) {
        for (j = 0; j < P4EST_FACES; j++) {
          adjncy[l++] = (int) conn->tree_to_tree[P4EST_FACES * i + j];
        }
        if (conntype == (P4EST_DIM - 1)) {
          for (j = 0; j < P4EST_CHILDREN; j++) {
            /* add the number of strict corner neighbors */
            p4est_find_corner_transform (conn, (p4est_topidx_t) i, j, &ci);
            for (zz = 0; zz < cta->elem_count; zz++) {
              ct = p4est_corner_array_index (cta, zz);
              adjncy[l++] = (int) ct->ntree;
            }
          }
        }
#ifdef P4_TO_P8
        if (conntype >= 1) {
          /* add the number of strict edge neighbors */
          for (j = 0; j < P8EST_EDGES; j++) {
            p8est_find_edge_transform (conn, (p4est_topidx_t) i, j, &ei);
            for (zz = 0; zz < eta->elem_count; zz++) {
              et = p8est_edge_array_index (eta, zz);
              adjncy[l++] = (int) et->ntree;
            }
          }
        }
#endif
        P4EST_ASSERT (l == xadj[i + 1]);
      }

      P4EST_ASSERT (l == totaldeg);

      if (conntype == (P4EST_DIM - 1)) {
        sc_array_reset (cta);
      }
#ifdef P4_TO_P8
      if (conntype >= 1) {
        sc_array_reset (eta);
      }
#endif
    }

    P4EST_GLOBAL_INFO ("Entering metis\n");
    /* now call metis */
    METIS_PartGraphVKway (&n, xadj, adjncy, NULL, NULL, &wgtflag, &numflag,
                          &k, options, &volume, part);
    P4EST_GLOBAL_INFO ("Done metis\n");

    P4EST_GLOBAL_STATISTICSF ("metis volume %d\n", volume);

    P4EST_FREE (xadj);
    P4EST_FREE (adjncy);
  }

  /* broadcast part to every process: this is expensive, should probably think
   * of a better way to do this */
  MPI_Bcast (part, n, MPI_INT, 0, comm);

  /* now that everyone has part, each process computes the renumbering
   * for itself*/
  newid = sc_array_new_size (sizeof (size_t), (size_t) n);
  sorter = sc_array_new_size (2 * sizeof (int), (size_t) n);
  for (i = 0; i < n; i++) {
    ip = (int *) sc_array_index (sorter, i);
    ip[0] = part[i];
    ip[1] = i;
  }
  P4EST_FREE (part);

  /* sort current index by partition given */
  /* this will be the same on every process because the comparison operation
   * does not allow equality between different trees */
  sc_array_sort (sorter, reorder_comp);
  for (i = 0; i < n; i++) {
    ip = (int *) sc_array_index (sorter, i);
    zp = (size_t *) sc_array_index (newid, ip[1]);
    *zp = i;
  }
  sc_array_destroy (sorter);

  /* first we change the entries in the various tables */

  /* tree_to_tree */
  for (i = 0; i < n; i++) {
    for (j = 0; j < P4EST_FACES; j++) {
      l = (int) conn->tree_to_tree[P4EST_FACES * i + j];
      zp = (size_t *) sc_array_index (newid, l);
      conn->tree_to_tree[P4EST_FACES * i + j] = (p4est_topidx_t) (*zp);
    }
  }

#ifdef P4_TO_P8
  /* edge_to_tree */
  if (conn->edge_to_tree != NULL) {
    count = (int) conn->ett_offset[conn->num_edges];
    for (i = 0; i < count; i++) {
      l = (int) conn->edge_to_tree[i];
      zp = (size_t *) sc_array_index (newid, l);
      conn->edge_to_tree[i] = (p4est_topidx_t) (*zp);
    }
  }
#endif

  /* corner_to_tree */
  if (conn->corner_to_tree != NULL) {
    count = (int) conn->ctt_offset[conn->num_corners];
    for (i = 0; i < count; i++) {
      l = (int) conn->corner_to_tree[i];
      zp = (size_t *) sc_array_index (newid, l);
      conn->corner_to_tree[i] = (p4est_topidx_t) (*zp);
    }
  }

  /* now we reorder the various tables via in-place permutation */

  /* tree_to_vertex */
  sc_array_init_data (&array_view, conn->tree_to_vertex,
                      P4EST_CHILDREN * sizeof (p4est_topidx_t), n);
  sc_array_permute (&array_view, newid, 1);

  /* tree_to_tree */
  sc_array_init_data (&array_view, conn->tree_to_tree,
                      P4EST_FACES * sizeof (p4est_topidx_t), n);
  sc_array_permute (&array_view, newid, 1);

  /* tree_to_face */
  sc_array_init_data (&array_view, conn->tree_to_face,
                      P4EST_FACES * sizeof (int8_t), n);
  sc_array_permute (&array_view, newid, 1);

#ifdef P4_TO_P8
  /* tree_to_edge */
  if (conn->tree_to_edge != NULL) {
    sc_array_init_data (&array_view, conn->tree_to_edge,
                        P8EST_EDGES * sizeof (p4est_topidx_t), n);
    sc_array_permute (&array_view, newid, 1);
  }
#endif

  /* tree_to_corner */
  if (conn->tree_to_corner != NULL) {
    sc_array_init_data (&array_view, conn->tree_to_corner,
                        P4EST_CHILDREN * sizeof (p4est_topidx_t), n);
    sc_array_permute (&array_view, newid, 1);
  }

  P4EST_ASSERT (p4est_connectivity_is_valid (conn));

  sc_array_destroy (newid);
}
#endif

#ifndef P4_TO_P8
int
p4est_connect_type_int (p4est_connect_type_t btype)
{
  switch (btype) {
  case P4EST_CONNECT_FACE:
    return 1;
  case P4EST_CONNECT_CORNER:
    return 2;
  default:
    SC_ABORT_NOT_REACHED ();
  }
}

const char         *
p4est_connect_type_string (p4est_connect_type_t btype)
{
  switch (btype) {
  case P4EST_CONNECT_FACE:
    return "FACE";
  case P4EST_CONNECT_CORNER:
    return "CORNER";
  default:
    SC_ABORT_NOT_REACHED ();
  }
}
#endif /* !P4_TO_P8 */
