#include "config_manager_tlv.h"
#include "config_manager_dbg.h"
#include <sstream>
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
// xxx after each read, check how much was read.
// xxx sanity check: L read from the file should never be 0
//
unsigned int cm_composite_tlv::load(FILE * fp, uint8_t * pItem) const
{
    cm_item_len_t  tlvLen;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);
    cm_item_len_t bytesRead = sizeof(tlvLen);

    DBG_PRT("composite load %d bytes to %p\n", tlvLen, pItem);

    bool         first   = true; // Is this the first component item?
    unsigned int itemIdx;        // number of items read of a given type, i.e. offset in the item array

    // While enough unread bytes remain of the composite for T+L fields of a component.
    // bytesRead already includes the L field of the composite item, so we don't add sizeof(L) here.
    while (bytesRead + sizeof(cm_item_id_t) <= tlvLen) 
    {        
        cm_item_id_t  compId, prevCompId;

        fread(&compId, sizeof(compId), 1, fp);
        bytesRead += sizeof(compId);

        DBG_PRT("load component ID %d\n", compId);

        // New item type (or the 1st one), so reset array index
        if (first || (compId != prevCompId))
        {                
            first = false;
            itemIdx = 0;
        }

        const cm_aggregate * pAggr;
        uint8_t *            pComponentItem;

        if (!pDesc->getComponentItem(compId, itemIdx, &pAggr, pItem, &pComponentItem))
        {
            // skip because we couldn't find the item, index is out of range, or couldn't malloc
            bytesRead += skipItem(fp);
        }
        else
        {        
            const cm_descriptor * pComponentDesc = pAggr->pData->pDesc;

            bytesRead += pComponentDesc->loadFromTlv(fp, pComponentItem);
            itemIdx++;
        }

        DBG_PRT("composite '%s': %d read\n", pDesc->getName().c_str(), bytesRead);
        prevCompId = compId;
    }
    
    // Sanity check on coherence of the data read from the TLV file: xxx should we abort the read?
    if (bytesRead != tlvLen + sizeof(tlvLen))
    {
        cout << "'" << pDesc->getName() << "' contains " << bytesRead - sizeof(tlvLen) << ", not "
             << tlvLen <<"!" << endl;
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


// Having read the Type (ID) of an item from the TLV file, skip L and V.
// @return number of bytes moved ahead in the file
//
unsigned int cm_composite_tlv::skipItem(FILE * fp) const
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
unsigned int cm_simple_tlv::load(FILE * fp, uint8_t * pItem) const
{
    cm_item_len_t tlvLen;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);

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
        fread(pItem, tlvLen, 1, fp);
    }
    return sizeof(tlvLen) + tlvLen;
}

