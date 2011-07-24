/// 
// Type-Length-Value format of data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in NVRAM.
#include "config_manager_tlv.h"
#include "config_manager_dbg.h"
#include <sstream>
#include <string.h> // memcpy

using namespace std;


////////////////////////////////////////////////////////////////////////////////
//
// cm_composite_tlv
//
////////////////////////////////////////////////////////////////////////////////

/// Write item's TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx add param, bufsize, to do a buffer overflow check.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_composite_tlv::write(const uint8_t *pItem, uint8_t ** ppBuf) const
{
    // The L field in TLV excludes the item's T+L fields
    cm_item_len_t componentLen = getLen(pItem) - sizeof(pDesc->getLen()) - sizeof(cm_item_len_t);
    
    // First write own Type and Length
    memcpy(*ppBuf, pDesc->getIdPtr(), sizeof(cm_item_id_t)); // write Type (i.e. the ID)
    *ppBuf += sizeof(cm_item_id_t);                          // advance the memory pointer
    memcpy(*ppBuf, &componentLen, sizeof(componentLen));     // write Length (of Value to follow)
    *ppBuf += sizeof(componentLen);                          // advance the memory pointer
    
    // Now write V, which is the TLVs of all components
    for (int i = 0; i < pDesc->getAggrCount(); i++)
    {            
        const cm_aggregate *  pAggr          = pDesc->getAggr(i);
        const uint8_t *       pFirstItem     = pAggr->getFirstItem(pItem);
        unsigned int          itemCount      = pAggr->getCount(pItem);
        const cm_descriptor * pComponentDesc = pAggr->pData->pDesc;

        for (unsigned j = 0; j < itemCount; j++)
        {
            pComponentDesc->writeTlv(pFirstItem + j * pComponentDesc->getLen(), ppBuf);
        }
    }
}


// This method is called if the TLV field T (the item ID) matches,
// so it's not checked here again.
// This method reads L, and moves forward in the file by that many bytes,
// using what it finds in the file to initialize the object's configurable items.
// @return number of bytes read
//
// xxx sanity check: L read from the file should never be 0
//
unsigned int cm_composite_tlv::load(FILE * fp, uint8_t * pItem, t_cm_result & res) const
{
    cm_item_len_t tlvLen;
    cm_item_len_t bytesRead = 0;

    if (fread(&tlvLen, sizeof(tlvLen), 1, fp) != 1)
    {
        res = CM_READ_FAIL;
        return bytesRead;
    }

    bytesRead = sizeof(tlvLen);

    DBG_PRT("composite load %d bytes to %p\n", tlvLen, pItem);

    bytesRead += loadComponents(fp, pItem, tlvLen, res);
    
    // Sanity check on coherence of the data read from the TLV file
    if (bytesRead != tlvLen + sizeof(tlvLen))
    {
        cout << "'" << pDesc->getName() << "' contains " << bytesRead - sizeof(tlvLen) << ", not "
             << tlvLen <<"!" << endl;
        res = CM_INCOHERENT_DATA;
    }
    return bytesRead;
}


// Load components of this composite.
// @return number of bytes read
//
// xxx sanity check: L read from the file should never be 0
//
unsigned int cm_composite_tlv::loadComponents(FILE * fp, 
                                              uint8_t * pItem,
                                              cm_item_len_t bytesToRead,
                                              t_cm_result & res) const
{
    bool          first = true; // Is this the first component item?
    cm_item_len_t bytesRead = 0;

    // While enough unread bytes remain for T+L fields of a component.
    while (bytesRead + sizeof(cm_item_id_t) + sizeof(cm_item_len_t) <= bytesToRead)
    {        
        bytesRead += loadComponent(fp, pItem, first, res);

        if (res != CM_SUCCESS)
        {
            return bytesRead;
        }
        
        first = false;
        DBG_PRT("composite '%s': %d read\n", pDesc->getName().c_str(), bytesRead);
    }
    return bytesRead;
}


/// Return total length of TLV item, the number of bytes in T + L + V.
cm_item_len_t cm_composite_tlv::getLen(const uint8_t * pItem) const
{
    cm_item_len_t tlvLen = sizeof(cm_item_id_t) + sizeof(cm_item_len_t);

    // For each aggregate, and for each of the array of items under it...
    for (int i = 0; i < pDesc->getAggrCount(); i++)
    {
        const cm_aggregate *  pAggr           = pDesc->getAggr(i);
        const uint8_t *       pFirstItem      = pAggr->getFirstItem(pItem);
        unsigned int          itemCount       = pAggr->getCount(pItem);
        const cm_descriptor * pComponentDesc  = pAggr->pData->pDesc;

        for (unsigned j = 0; j < itemCount; j++)
        {
            tlvLen += pComponentDesc->getTlvLen(pFirstItem + j * pComponentDesc->getLen());
        }
    }
    return tlvLen;
}


//
// @param fp, input, file to load from
// @param pItem, input, pointer to base of RAM where value read from file should be loaded
// @param first, input, is this first read within component, meaning input lastCompId is invalid?
// @param itemIdx, input/output, index representing offset in RAM
// @param lastCompId, input/output, ID read from file, valid if res is CM_SUCCESS
// @param res, output, CM_SUCCESS, or CM_READ_FAIL if unable to read from file.
//        Note that CM_SUCCESS is returned even if we don't load an item because it
//        is not as expected, e.g. too many instances, unknown ID, etc xxx
// @return number of bytes read from fp
//
unsigned int cm_composite_tlv::loadComponent(FILE * fp,
                                             uint8_t * pItem,
                                             bool first,
                                             t_cm_result & res) const
{
    static unsigned int itemIdx;       // 0-based number of next instance of given compId to read
    cm_item_len_t       bytesRead = 0; // Number of bytes read by this method
    cm_item_id_t        compId;        // compId read by this method
    #if 0
    static cm_item_id_t lastCompId;    // previous compId read
    #else
    cm_item_id_t lastCompId;    // previous compId read
    #endif
    
    if (fread(&compId, sizeof(compId), 1, fp) != 1)
    {
        res = CM_READ_FAIL;
        return bytesRead;
    }

    bytesRead += sizeof(compId);

    DBG_PRT("loadComponent ID %d\n", compId);

    if (first || (lastCompId != compId))
    {
        // First read of this component ID, i.e. first item in an array
        lastCompId = compId;
        itemIdx = 0;
    }

    const cm_aggregate * pAggr;
    uint8_t *            pComponentItem;

    if (!pDesc->getComponentItem(compId, itemIdx++, &pAggr, pItem, &pComponentItem))
    {
        // skip because we couldn't find the ID, index is out of range, or couldn't malloc
        bytesRead += skipItem(fp, res);
    }
    else
    {        
        bytesRead += pAggr->pData->pDesc->loadFromTlv(fp, pComponentItem, res);
    }
    return bytesRead;
}


// Having read the Type (ID) of an item from the TLV file, skip L and V.
// @return number of bytes moved ahead in the file
// xxx write to res parameter
unsigned int cm_composite_tlv::skipItem(FILE * fp, t_cm_result & res) const
{
    cm_item_len_t tlvLen;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);
    cm_item_len_t bytesRead = sizeof(tlvLen);

    fseek(fp, tlvLen, SEEK_CUR);
    return bytesRead += tlvLen;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_simple_tlv
//
////////////////////////////////////////////////////////////////////////////////


/// Write TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_simple_tlv::write(const uint8_t *pItem, uint8_t ** ppBuf) const
{
    memcpy(*ppBuf, pDesc->getIdPtr(), sizeof(cm_item_id_t));   // write Type (i.e. the ID)
    *ppBuf += sizeof(cm_item_id_t);                            // advance the memory pointer

    memcpy(*ppBuf, pDesc->getLenPtr(), sizeof(cm_item_len_t)); // write Length
    *ppBuf += sizeof(cm_item_len_t);                           // advance the memory pointer

    memcpy(*ppBuf, pItem, pDesc->getLen());                    // write Value
    *ppBuf += pDesc->getLen();
}


/// Return total length of TLV item, the number of bytes in T + L + V.
//  (For a simple item, there's no dependency on pItem, the RAM contents.)
cm_item_len_t cm_simple_tlv::getLen(const uint8_t * pItem) const
{
    return sizeof(cm_item_id_t) + sizeof(cm_item_len_t) + pDesc->getLen();
}


// This method is called if the TLV field T (the item ID) matches,
// so it's not checked here again.
// This method reads L, and moves forward in the file by that many bytes,
// using what it finds in the file to initialize the object's configurable items.
// xxx after each read, check how much was read.
unsigned int cm_simple_tlv::load(FILE * fp, uint8_t * pItem, t_cm_result & res) const
{
    cm_item_len_t tlvLen;
    
    if (fread(&tlvLen, sizeof(tlvLen), 1, fp) != 1)
    {
        res = CM_READ_FAIL;
        goto exit;
    }

    DBG_PRT("load simple %d bytes to %p\n", tlvLen, pItem);

    if (tlvLen != pDesc->getLen())
    {
        // Item larger than expected: we don't truncate, we leave
        // the item as unchanged, but move forward in the file.
        // xxx Could we handle the case where len > tlvLen?  No, for big-endian
        // systems we'd have to know if we were reading an integer or not.
        cout << "TLV len " << tlvLen << ", expected " << pDesc->getLen() << endl;

        fseek(fp, tlvLen, SEEK_CUR);
    }
    else
    {
        if (fread(pItem, tlvLen, 1, fp) != 1)
        {
            res = CM_READ_FAIL;
            goto exit;
        }
    }

exit:
    return sizeof(tlvLen) + tlvLen;
}

