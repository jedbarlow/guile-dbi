/* guile-dbd-postgresql.c - Main source file
 * Copyright (C) 2004, 2005, 2006,2008 Free Software Foundation, Inc.
 * Written by Maurizio Boriani <baux@member.fsf.org>
 * Maintained by Linas Vepstas <linasvepstas@gmail.com>
 *
 * This file is part of the guile-dbd-postgresql.
 *
 * The guile-dbd-postgresql is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The guile-dbd-postgresql is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * */


#include <guile-dbi/guile-dbi.h>
#include <libguile.h>
#include <postgresql/libpq-fe.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/* Function prototypes and structures */
void __postgresql_make_g_db_handle(gdbi_db_handle_t* dbh);
void __postgresql_close_g_db_handle(gdbi_db_handle_t* dbh);
void __postgresql_query_g_db_handle(gdbi_db_handle_t* dbh, char* query);
SCM __postgresql_getrow_g_db_handle(gdbi_db_handle_t* dbh);


typedef struct
{
  PGconn*   pgsql;
  PGresult* res;
  int       lget;
  int       retn;
} gdbi_pgsql_ds_t;



void
__postgresql_make_g_db_handle(gdbi_db_handle_t* dbh)
{
  char sep=':';
  int  items=0;
  gdbi_pgsql_ds_t* pgsqlP = NULL;
  SCM cp_list = SCM_EOL;

  if (scm_equal_p(scm_string_p(dbh->constr), SCM_BOOL_F) == SCM_BOOL_T)
    {
      /* todo: error msg to be translated */
      dbh->status = scm_cons(scm_from_int(1),
                      scm_from_locale_string("missing connection string"));
      return;
    }

  cp_list = scm_string_split(dbh->constr,SCM_MAKE_CHAR(sep));

  items = scm_to_int(scm_length(cp_list));
  if (items >= 5 && items < 8)
    {
      char* port = NULL;
      char* user, *pass, *db, *ctyp, *loc;

      pgsqlP = (gdbi_pgsql_ds_t*) malloc(sizeof(gdbi_pgsql_ds_t));
      pgsqlP->retn = 0;

      if (pgsqlP == NULL)
        {
          dbh->status = scm_cons(scm_from_int(errno),
                                 scm_from_locale_string(strerror(errno)));
          return;
        }

      user = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(0)));
      pass = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(1)));
      db   = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(2)));
      ctyp = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(3)));
      loc  = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(4)));

      pgsqlP->pgsql = NULL;
      pgsqlP->res   = NULL;

      if (strcmp(ctyp,"tcp") == 0)
        {
          port  = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(5)));
          pgsqlP->pgsql = (PGconn*) PQsetdbLogin(loc,port,NULL,NULL,db,user,pass);
          if (items == 7)
            {
              char* sretn  = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(6)));
              pgsqlP->retn = atoi(sretn);
         free (sretn);
            }
        }
      else
        {
          pgsqlP->pgsql = (PGconn*) PQsetdbLogin(loc,NULL,NULL,NULL,db,user,pass);
          if (items == 6)
            {
              char* sretn  = scm_to_locale_string(scm_list_ref(cp_list,scm_from_int(5)));
              pgsqlP->retn = atoi(sretn);
              free (sretn);
            }
        }

      /* free resources */
      if (user)
        {
          free(user);
        }
      if (pass)
        {
          free(pass);
        }
      if (db)
        {
          free(db);
        }
      if (ctyp)
        {
          free(ctyp);
        }
      if (loc)
        {
          free(loc);
        }
      if (port)
        {
          free(port);
        }

      if (PQstatus(pgsqlP->pgsql) == CONNECTION_BAD)
        {
          dbh->status = scm_cons(scm_from_int(1),
                     scm_from_locale_string(PQerrorMessage(pgsqlP->pgsql)));
          PQfinish(pgsqlP->pgsql);
          pgsqlP->pgsql = NULL;
          free(pgsqlP);
          dbh->db_info = NULL;
          dbh->closed = SCM_BOOL_T;
          return;
        }
      else
        {
          /* todo: error msg to be translated */
          dbh->status = scm_cons(scm_from_int(0),
                          scm_from_locale_string("db connected"));
          dbh->db_info = pgsqlP;
          dbh->closed = SCM_BOOL_F;
          return;
        }
    }
  else
    {
      /* todo: error msg to be translated */
      dbh->status = scm_cons(scm_from_int(1),
                     scm_from_locale_string("invalid connection string"));
      dbh->db_info = NULL;
      dbh->closed = SCM_BOOL_T;
      return;
    }

  return;
}



void
__postgresql_close_g_db_handle(gdbi_db_handle_t* dbh)
{
  gdbi_pgsql_ds_t* pgsqlP = (gdbi_pgsql_ds_t*)dbh->db_info;
  if (pgsqlP == NULL)
    {
      if (dbh->in_free) return; /* don't scm anything if in GC */
      /* todo: error msg to be translated */
      dbh->status = scm_cons(scm_from_int(1),
                 scm_from_locale_string("dbd info not found"));
      return;
    }

  /* Don't scm anything if we are in garbage collection */
  if (0 == dbh->in_free)
    {
      if (NULL == pgsqlP->pgsql)
        {
          /* todo: error msg to be translated */
          dbh->status = scm_cons(scm_from_int(1),
                 scm_from_locale_string("dbi connection already closed"));
        }
      else
        {
          dbh->status = scm_cons(scm_from_int(0),
                      scm_from_locale_string("dbi closed"));
        }
    }

  PQfinish(pgsqlP->pgsql);
  if (pgsqlP->res)
    {
      PQclear(pgsqlP->res);
      pgsqlP->res = NULL;
    }

  free(dbh->db_info);
  dbh->db_info = NULL;

  dbh->closed = SCM_BOOL_T;
}



void
__postgresql_query_g_db_handle(gdbi_db_handle_t* dbh, char* query)
{
  gdbi_pgsql_ds_t* pgsqlP = NULL;
  int err, i, bpid;

  if (dbh->db_info == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = scm_cons(scm_from_int(1),
                   scm_from_locale_string("invalid dbi connection"));
      return;
    }

  if (query == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = scm_cons(scm_from_int(1),
                 scm_from_locale_string("invalid dbi query"));
      return;
    }

  pgsqlP = (gdbi_pgsql_ds_t*)dbh->db_info;

  if (pgsqlP->res)
    {
      PQclear(pgsqlP->res);
      pgsqlP->res = NULL;
    }

#if 0
  if (PQresultStatus(pgsqlP->res) == PGRES_FATAL_ERROR)
    {
      for (i = 0, bpid=0; i < pgsqlP->retn && bpid == 0; i++)
        {
          PQreset(pgsqlP->pgsql);
          bpid = PQbackendPID(pgsqlP->pgsql);
        }
    }
#endif
  err = PQsendQuery(pgsqlP->pgsql, query);

  if (err == 1)
    {
      dbh->status = scm_cons(scm_from_int(0),
                    scm_from_locale_string("query ok"));
      pgsqlP->lget = 0;
    }
  else
    {
      dbh->status = scm_cons(scm_from_int(1),
              scm_from_locale_string(PQerrorMessage(pgsqlP->pgsql)));
    }
}



SCM
__postgresql_getrow_g_db_handle(gdbi_db_handle_t* dbh)
{
  gdbi_pgsql_ds_t* pgsqlP = NULL;
  SCM retrow = SCM_EOL;
  int fnum,f ;

  if (dbh->db_info == NULL)
    {
      /* todo: error msg to be translated */
      dbh->status = scm_cons(scm_from_int(1),
                   scm_from_locale_string("invalid dbi connection"));
      return (SCM_BOOL_F);
    }

  pgsqlP = (gdbi_pgsql_ds_t*)dbh->db_info;
  if (NULL == pgsqlP->res)
      pgsqlP->res = PQgetResult(pgsqlP->pgsql);

  if (NULL == pgsqlP->res)
    {
      dbh->status = scm_cons(scm_from_int(0),
                                   scm_from_locale_string("row end"));
      pgsqlP->lget = 0;
      return SCM_BOOL_F;
    }

  if (pgsqlP->lget == PQntuples(pgsqlP->res))
    {
      pgsqlP->lget = 0;
      PQclear(pgsqlP->res);
      pgsqlP->res = PQgetResult(pgsqlP->pgsql);
    }

  /* Check result status before unpacking the data! */
  switch (PQresultStatus(pgsqlP->res))
    {
    case PGRES_BAD_RESPONSE:
    case PGRES_NONFATAL_ERROR:
    case PGRES_FATAL_ERROR:
      if (pgsqlP->res == NULL)
        {
          dbh->status = scm_cons(scm_from_int(0),
                                scm_from_locale_string("row end"));
        }
      else
        {
          dbh->status = scm_cons(scm_from_int(1),
               scm_from_locale_string(PQresStatus(PQresultStatus(pgsqlP->res))));
        }
      return SCM_BOOL_F;

    case PGRES_EMPTY_QUERY:
    case PGRES_COMMAND_OK:
    case PGRES_TUPLES_OK:
    case PGRES_COPY_OUT:
    case PGRES_COPY_IN:
      dbh->status = scm_cons(scm_from_int(0),
            scm_from_locale_string(PQresStatus(PQresultStatus(pgsqlP->res))));
      break;

    default:
      dbh->status = scm_cons(scm_from_int(0),
            scm_from_locale_string("unknown return query status"));
      break;
    }

  fnum = PQnfields(pgsqlP->res);
  for (f = 0; f<fnum; f++)
    {
      SCM value;
      int type = PQftype(pgsqlP->res, f);
      if ((type >= 20 && type <= 24) ||
          type == 1700               ||
          type == 26                  )
        {
          const char * vstr = PQgetvalue(pgsqlP->res, pgsqlP->lget,f);
          value = scm_from_long(atoi(vstr));
        }
      else if (type == 700 ||
          type == 701            )
        {
          const char * vstr = PQgetvalue(pgsqlP->res, pgsqlP->lget, f);
          value = scm_from_double(atof(vstr));
        }
      else if (type == 18 ||
               type == 19 ||
               type == 25 ||
               type == 702 ||
               (type >= 1042 && type <= 1114))
        {
          const char * vstr = PQgetvalue(pgsqlP->res, pgsqlP->lget, f);
          value = scm_from_locale_string(vstr);
        }
      else
        {
          dbh->status = scm_cons(scm_from_int(1),
                 scm_from_locale_string("unknown field type"));
          pgsqlP->lget++;
          return SCM_EOL;
        }

      retrow = scm_append(scm_list_2(retrow,
          scm_list_1(scm_cons(scm_from_locale_string(PQfname(pgsqlP->res,f)),
                                              value))));
    }

  pgsqlP->lget++;

  if (retrow == SCM_EOL) retrow = SCM_BOOL_F;
  return retrow;
}
