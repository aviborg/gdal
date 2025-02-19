/**********************************************************************
 *
 * Project:  GML Reader
 * Purpose:  Implementation of GMLFeature.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 **********************************************************************
 * Copyright (c) 2002, Frank Warmerdam
 * Copyright (c) 2010-2013, Even Rouault <even dot rouault at spatialys.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "cpl_port.h"
#include "gmlreader.h"

#include <cstdio>

#include "cpl_conv.h"
#include "cpl_error.h"
#include "cpl_minixml.h"
#include "cpl_string.h"


/************************************************************************/
/*                             GMLFeature()                             */
/************************************************************************/

GMLFeature::GMLFeature( GMLFeatureClass *poClass ) :
    m_poClass(poClass),
    m_pszFID(nullptr),
    m_nPropertyCount(0),
    m_pasProperties(nullptr),
    m_nGeometryCount(0),
    m_papsGeometry(m_apsGeometry),  // TODO(schwehr): Allowed in init list?
    m_papszOBProperties(nullptr)
{
    m_apsGeometry[0] = nullptr;
    m_apsGeometry[1] = nullptr;
}

/************************************************************************/
/*                            ~GMLFeature()                             */
/************************************************************************/

GMLFeature::~GMLFeature()

{
    CPLFree(m_pszFID);

    for( int i = 0; i < m_nPropertyCount; i++ )
    {
        const int nSubProperties = m_pasProperties[i].nSubProperties;
        if (nSubProperties == 1)
        {
            CPLFree(m_pasProperties[i].aszSubProperties[0]);
        }
        else if (nSubProperties > 1)
        {
            for( int j = 0; j < nSubProperties; j++)
                CPLFree(m_pasProperties[i].papszSubProperties[j]);
            CPLFree(m_pasProperties[i].papszSubProperties);
        }
    }

    if (m_nGeometryCount == 1)
    {
        CPLDestroyXMLNode(m_apsGeometry[0]);
    }
    else if (m_nGeometryCount > 1)
    {
        for( int i = 0; i < m_nGeometryCount; i++ )
            CPLDestroyXMLNode(m_papsGeometry[i]);
        CPLFree(m_papsGeometry);
    }

    CPLFree(m_pasProperties);
    CSLDestroy(m_papszOBProperties);
}

/************************************************************************/
/*                               SetFID()                               */
/************************************************************************/

void GMLFeature::SetFID( const char *pszFID )

{
    CPLFree(m_pszFID);
    if( pszFID != nullptr )
        m_pszFID = CPLStrdup(pszFID);
    else
        m_pszFID = nullptr;
}

/************************************************************************/
/*                        SetPropertyDirectly()                         */
/************************************************************************/

void GMLFeature::SetPropertyDirectly( int iIndex, char *pszValue )

{
    CPLAssert(pszValue);
    if( iIndex >= m_nPropertyCount )
    {
        const int nClassPropertyCount = m_poClass->GetPropertyCount();
        m_pasProperties = static_cast<GMLProperty *>(CPLRealloc(
            m_pasProperties, sizeof(GMLProperty) * nClassPropertyCount));
        for( int i = 0; i < m_nPropertyCount; i++ )
        {
            // Make sure papszSubProperties point to the right address in case
            // m_pasProperties has been relocated.
            if (m_pasProperties[i].nSubProperties <= 1)
                m_pasProperties[i].papszSubProperties =
                    m_pasProperties[i].aszSubProperties;
        }
        for( int i = m_nPropertyCount; i < nClassPropertyCount; i++ )
        {
            m_pasProperties[i].nSubProperties = 0;
            m_pasProperties[i].papszSubProperties =
                m_pasProperties[i].aszSubProperties;
            m_pasProperties[i].aszSubProperties[0] = nullptr;
            m_pasProperties[i].aszSubProperties[1] = nullptr;
        }
        m_nPropertyCount = nClassPropertyCount;
    }

    GMLProperty *psProperty = &m_pasProperties[iIndex];
    const int nSubProperties = psProperty->nSubProperties;
    if (nSubProperties == 0)
    {
        psProperty->aszSubProperties[0] = pszValue;
    }
    else if (nSubProperties == 1)
    {
        psProperty->papszSubProperties = static_cast<char **>(
            CPLMalloc(sizeof(char *) * (nSubProperties + 2)));
        psProperty->papszSubProperties[0] = psProperty->aszSubProperties[0];
        psProperty->aszSubProperties[0] = nullptr;
        psProperty->papszSubProperties[nSubProperties] = pszValue;
        psProperty->papszSubProperties[nSubProperties + 1] = nullptr;
    }
    else
    {
        psProperty->papszSubProperties = static_cast<char **>(
            CPLRealloc(psProperty->papszSubProperties,
                       sizeof(char *) * (nSubProperties + 2)));
        psProperty->papszSubProperties[nSubProperties] = pszValue;
        psProperty->papszSubProperties[nSubProperties + 1] = nullptr;
    }
    psProperty->nSubProperties++;
}

/************************************************************************/
/*                                Dump()                                */
/************************************************************************/

void GMLFeature::Dump( CPL_UNUSED FILE * fp )
{
    printf("GMLFeature(%s):\n", m_poClass->GetName());  /*ok*/

    if( m_pszFID != nullptr )
        printf("  FID = %s\n", m_pszFID);  /*ok*/

    for( int i = 0; i < m_nPropertyCount; i++ )
    {
        const GMLProperty *psGMLProperty = GetProperty(i);
        printf("  %s = ", m_poClass->GetProperty(i)->GetName()); /*ok*/
        if( psGMLProperty != nullptr )
        {
            for ( int j = 0; j < psGMLProperty->nSubProperties; j ++)
            {
                if (j > 0)
                    printf(", ");  /*ok*/
                printf("%s", psGMLProperty->papszSubProperties[j]);  /*ok*/
            }
            printf("\n");/*ok*/
        }
    }

    for( int i = 0; i < m_nGeometryCount; i++ )
    {
        char *pszXML = CPLSerializeXMLTree(m_papsGeometry[i]);
        printf("  %s\n", pszXML);  /*ok*/
        CPLFree(pszXML);
    }
}

/************************************************************************/
/*                        SetGeometryDirectly()                         */
/************************************************************************/

void GMLFeature::SetGeometryDirectly( CPLXMLNode* psGeom )

{
    if (m_apsGeometry[0] != nullptr)
        CPLDestroyXMLNode(m_apsGeometry[0]);
    m_nGeometryCount = 1;
    m_apsGeometry[0] = psGeom;
}

/************************************************************************/
/*                        SetGeometryDirectly()                         */
/************************************************************************/

void GMLFeature::SetGeometryDirectly( int nIdx, CPLXMLNode* psGeom )

{
    if( nIdx == 0 && m_nGeometryCount <= 1 )
    {
        SetGeometryDirectly(psGeom);
        return;
    }
    else if( nIdx > 0 && m_nGeometryCount <= 1 )
    {
        m_papsGeometry =
            static_cast<CPLXMLNode **>(CPLMalloc(2 * sizeof(CPLXMLNode *)));
        m_papsGeometry[0] = m_apsGeometry[0];
        m_papsGeometry[1] = nullptr;
        m_apsGeometry[0] = nullptr;
    }

    if( nIdx >= m_nGeometryCount )
    {
        m_papsGeometry = static_cast<CPLXMLNode **>(
            CPLRealloc(m_papsGeometry, (nIdx + 2) * sizeof(CPLXMLNode *)));
        for( int i = m_nGeometryCount; i <= nIdx + 1; i++ )
            m_papsGeometry[i] = nullptr;
        m_nGeometryCount = nIdx + 1;
    }
    if (m_papsGeometry[nIdx] != nullptr)
        CPLDestroyXMLNode(m_papsGeometry[nIdx]);
    m_papsGeometry[nIdx] = psGeom;
}

/************************************************************************/
/*                          GetGeometryRef()                            */
/************************************************************************/

const CPLXMLNode *GMLFeature::GetGeometryRef( int nIdx ) const
{
    if( nIdx < 0 || nIdx >= m_nGeometryCount )
        return nullptr;
    return m_papsGeometry[nIdx];
}

/************************************************************************/
/*                             AddGeometry()                            */
/************************************************************************/

void GMLFeature::AddGeometry( CPLXMLNode *psGeom )

{
    if (m_nGeometryCount == 0)
    {
        m_apsGeometry[0] = psGeom;
    }
    else if (m_nGeometryCount == 1)
    {
        m_papsGeometry = static_cast<CPLXMLNode **>(
            CPLMalloc((m_nGeometryCount + 2) * sizeof(CPLXMLNode *)));
        m_papsGeometry[0] = m_apsGeometry[0];
        m_apsGeometry[0] = nullptr;
        m_papsGeometry[m_nGeometryCount] = psGeom;
        m_papsGeometry[m_nGeometryCount + 1] = nullptr;
    }
    else
    {
        m_papsGeometry = static_cast<CPLXMLNode **>(CPLRealloc(
            m_papsGeometry, (m_nGeometryCount + 2) * sizeof(CPLXMLNode *)));
        m_papsGeometry[m_nGeometryCount] = psGeom;
        m_papsGeometry[m_nGeometryCount + 1] = nullptr;
    }
    m_nGeometryCount++;
}

/************************************************************************/
/*                           AddOBProperty()                            */
/************************************************************************/

void GMLFeature::AddOBProperty( const char *pszName, const char *pszValue )

{
    m_papszOBProperties =
        CSLAddNameValue(m_papszOBProperties, pszName, pszValue);
}

/************************************************************************/
/*                           GetOBProperty()                            */
/************************************************************************/

const char *GMLFeature::GetOBProperty( const char *pszName )

{
    return CSLFetchNameValue(m_papszOBProperties, pszName);
}

/************************************************************************/
/*                          GetOBProperties()                           */
/************************************************************************/

char **GMLFeature::GetOBProperties() { return m_papszOBProperties; }
