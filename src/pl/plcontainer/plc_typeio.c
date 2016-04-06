#include "postgres.h"
#include "utils/array.h"

#include "plcontainer.h"
#include "plc_typeio.h"
#include "message_fns.h"

Datum get_array_datum(plcArray *arr, plcTypeInfo *ret_type)
{
    Datum         dvalue;
    Datum	     *elems;
    ArrayType    *array = NULL;
    int          *lbs = NULL;
    int           i;
    MemoryContext oldContext;

    lbs = (int*)palloc(arr->meta->ndims * sizeof(int));
    for (i = 0; i < arr->meta->ndims; i++)
        lbs[i] = 1;

    elems = palloc(sizeof(Datum) * arr->meta->size);
    for (i = 0; i < arr->meta->size; i++) {
        elems[i] = Int64GetDatum( ((long long*)arr->data)[i] );
    }

    oldContext = MemoryContextSwitchTo(pl_container_caller_context);
    array = construct_md_array(elems,
                               arr->nulls,
                               arr->meta->ndims,
                               arr->meta->dims,
                               lbs,
                               ret_type->subTypes[0].typeOid,
                               ret_type->subTypes[0].typlen,
                               ret_type->subTypes[0].typbyval,
                               ret_type->subTypes[0].typalign);

    dvalue = PointerGetDatum(array);
    MemoryContextSwitchTo(oldContext);

    pfree(lbs);
    pfree(elems);

    return dvalue;
}

/*
void get_tuple_store( MemoryContext oldContext, MemoryContext messageContext,
        ReturnSetInfo *rsinfo, plcontainer_result res, int *isNull )
{
    AttInMetadata      *attinmeta;
    Tuplestorestate    *tupstore = tuplestore_begin_heap(true, false, work_mem);
    TupleDesc          tupdesc;
    HeapTuple    typetup,
                 tuple;
    Form_pg_type type;
    Oid          typeOid;
    int32        typeMod;

    char **values;
    int i,j;

     * TODO: Returning tuple, you will not have any tuple description for the
     * function returning setof record. This needs to be fixed *
     * get the requested return tuple description *
    if (rsinfo->expectedDesc != NULL)
        tupdesc = CreateTupleDescCopy(rsinfo->expectedDesc);
    else {
        elog(ERROR, "Functions returning 'record' type are not supported yet");
        *isNull = TRUE;
        return;
    }

    for (j = 0; j < res->cols; j++) {
        parseTypeString(res->types[j], &typeOid, &typeMod);
        typetup = SearchSysCache(TYPEOID, typeOid, 0, 0, 0);

        if (!HeapTupleIsValid(typetup)) {
            MemoryContextSwitchTo(oldContext);
            MemoryContextDelete(messageContext);
            elog(FATAL, "[plcontainer] Invalid heaptuple at result return");
            // This won`t run
            *isNull=TRUE;
        }

        type = (Form_pg_type)GETSTRUCT(typetup);

        strcpy(tupdesc->attrs[j]->attname.data, res->names[j]);
        tupdesc->attrs[j]->atttypid = typeOid;
        ReleaseSysCache(typetup);
    }

    attinmeta = TupleDescGetAttInMetadata(tupdesc);

    * OK, go to work *
    rsinfo->returnMode = SFRM_Materialize;
    MemoryContextSwitchTo(oldContext);

     *
     * SFRM_Materialize mode expects us to return a NULL Datum. The actual
     * tuples are in our tuplestore and passed back through
     * rsinfo->setResult. rsinfo->setDesc is set to the tuple description
     * that we actually used to build our tuples with, so the caller can
     * verify we did what it was expecting.
     *
    rsinfo->setDesc = tupdesc;

    for (i=0; i<res->rows;i++){

        values = palloc(sizeof(char *)* res->cols);
        for (j=0; j< res->cols;j++){
            values[j] = res->data[i][j].value;
        }

        * construct the tuple *
        tuple = BuildTupleFromCStrings(attinmeta, values);
        pfree(values);

        * switch to appropriate context while storing the tuple *
        oldContext = MemoryContextSwitchTo(messageContext);

        * now store it *
        tuplestore_puttuple(tupstore, tuple);

        MemoryContextSwitchTo(oldContext);
    }
    rsinfo->setResult = tupstore;
    MemoryContextSwitchTo(oldContext);
    MemoryContextDelete(messageContext);

    *isNull = TRUE;
}
*/
