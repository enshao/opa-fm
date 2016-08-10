/* BEGIN_ICS_COPYRIGHT7 ****************************************

Copyright (c) 2015, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

** END_ICS_COPYRIGHT7   ****************************************/

/* [ICS VERSION STRING: unknown] */

#ifndef _TOPOLOGY_H
#define _TOPOLOGY_H

#include <iba/ibt.h>

#include <iba/ipublic.h>
#include <iba/ib_pm.h>
#if !defined(VXWORKS) || defined(BUILD_DMC)
#include <iba/ib_dm.h>
#endif
#include <iba/stl_sm.h>
#include <iba/stl_sa.h>
#include <iba/stl_sd.h>
#include <iba/stl_pm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
//#include <ctype.h>
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <ixml_ib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CL_TIME_DIVISOR             1000000

struct ExpectedLink_s;
struct ExpectedNode_s;
struct ExpectedSM_s;
struct NodeData_s;
struct SystemData_s;
struct oib_port;

// Selection indicies for SCVL* tables
typedef enum { Enum_SCVLt, Enum_SCVLnt, Enum_SCVLr } ScvlEnum_t;

// selection of PortState for  FindPortState, includes all of IB_PORT_STATE
// plus these additional special searches
#define PORT_STATE_SEARCH_NOTACTIVE (IB_PORT_STATE_MAX+1)	// != active
#define PORT_STATE_SEARCH_INITARMED (IB_PORT_STATE_MAX+2)	// init or armed

// QOS information for a Port in the fabric
typedef struct QOSData_s {
	STL_VLARB_TABLE VLArbTable[4]; // One table per type, low, high, preempt, preempt matrix
	STL_SCSCMAP	*SC2SCMap;	// On SW ext ports will list SC2SC map for each
							// output port on SW
	STL_SLSCMAP	*SL2SCMap; // only defined for HFI and SW port 0
	STL_SCSLMAP	*SC2SLMap; // only defined for HFI and SW port 0
	STL_SCVLMAP	SC2VLMaps[3]; // VL_t, _nt, and _r; indicies given by ScvlEnum_t
} QOSData;

typedef STL_PORT_STATUS_RSP STL_PortStatusData_t;

// information about an IB Port in the fabric
// for switches a GUID and LID are only available for Port 0 of the switch
// for all other switch ports, LID is port 0 LID (LID for whole switch)
// and port GUID is NA and hence 0.
typedef struct PortData_s {
	cl_map_item_t	NodePortsEntry;	// NodeData.Ports, key is PortNum
	cl_map_item_t	AllLidsEntry;	// g_AllLids, key is LID (if GUID non-zero)
	LIST_ITEM		AllPortsEntry;	// g_AllPorts
	EUI64 PortGUID;					// 0 for all but port 0 of a switch
	struct PortData_s *neighbor;	// adjacent port this is cabled to
	struct NodeData_s *nodep;		// parent node
	uint8 PortNum;					// port number within Node
	uint8	from:1;					// is this the from port in link record
									// (avoids double reporting of links)
	uint8	PmaGotClassPortInfo:1;	// have issued a ClassPortInfo
	uint8	spare:6;
	uint32 rate;			// Active rate for this port
	STL_LID_32	EndPortLID;				// LID to get to device with this port
	STL_PORT_INFO	PortInfo;			// do not use LocalPortNum,use PortNum above
	STL_LED_INFO LedInfo;			//Led Info for this port
	IB_PATH_RECORD *pathp;			// Path Record to send to this port
	STL_PortStatusData_t *pPortStatus;
	struct ExpectedLink_s *elinkp;	// if supplied in topology input
	QOSData		*pQOS;				// optional QOS
	STL_PKEY_ELEMENT	*pPartitionTable;	// optional Partition Table

	union {
		struct {
			uint32		downlinkBasePaths;
			uint32		uplinkBasePaths;
			uint32		downlinkAllPaths;
			uint32		uplinkAllPaths;
		} fatTreeRoutes;	// for TabulateRoutes of fattree
		struct {
			uint32		recvBasePaths;
			uint32		xmitBasePaths;
			uint32		recvAllPaths;
			uint32		xmitAllPaths;
		} routes;			// for TabulateRoutes of any topology
	} analysisData;	// per port holding space for transient analysis data
	STL_BUFFER_CONTROL_TABLE *pBufCtrlTable;
	// 128 table entries allocate when needed
	STL_HFI_CONGESTION_CONTROL_TABLE_ENTRY *pCongestionControlTableEntries;
    uint16_t CCTI_Limit;

	// CableInfo is organized in 128-byte pages but is stored in
	// 64-byte half-pages
	// We only store STL_CIB_STD_START_ADDR to STL_CIB_STD_END_ADDR with
	// STL_CIB_STD_START_ADDR stored starting at pCableInfoData[0]
	uint8_t *pCableInfoData;
	void *context;					// application specific field
} PortData;

// additional information about cable for a link, from topology input
// we limit sizes to make output formatting easier
#define CABLE_LENGTH_STRLEN 10
#define CABLE_LABEL_STRLEN 20
#define CABLE_DETAILS_STRLEN 64
typedef struct CableData_s {
	char *length;	// user specified length
	char *label;	// user label on cable
	char *details;	// user description of cable
} CableData;

#define PORT_DETAILS_STRLEN 64
// port selector from topology input
typedef struct PortSelector_s {
	EUI64 PortGUID;					// 0 if not specified
	EUI64 NodeGUID;					// 0 if not specified
	uint8 PortNum;					// 0-255 are valid port numbers
	uint8 gotPortNum;				// 0 if PortNum not specified
	uint8 NodeType;					// 0 if not specified
	char *NodeDesc;					// NULL if not specified
	char *details;					// user description of port
} PortSelector;

#define LINK_DETAILS_STRLEN 64
// additional information about a link, from topology input
typedef struct ExpectedLink_s {
	LIST_ITEM		ExpectedLinksEntry;	// g_ExpectedLinks
	PortSelector *portselp1;	// input selector
	PortSelector *portselp2;	// input selector
	PortData *portp1;		// NULL if not found
	PortData *portp2;		// NULL if not found
	// the 4 fields below could become bit fields to save space if necessary
	// MTU=4 bits, rate=6 bits, internal=1 bit, matchLevel = 3 bits
	uint8 expected_rate;	// Expected Active rate for this link, IB_STATIC_RATE enum
	uint8 expected_mtu;	// Expected Active MTU for this link, an IB_MTU enum
	uint8 internal;	// Is link an internal link
	uint8 matchLevel; // degree of match with portp1 and 2, higher is better
					// used internally when matching ports to links to pick best
					// matches
	char *details;	// user description of link
	CableData CableData;	// user supplied info about cable
} ExpectedLink;

#define NODE_DETAILS_STRLEN 64
// additional information about a node, from topology input
typedef struct ExpectedNode_s {
	LIST_ITEM	ExpectedNodesEntry;	// g_ExpectedFIs, g_ExpectedSWs
	struct NodeData_s *nodep;		// NULL if not found
	EUI64 NodeGUID;					// 0 if not specified
	uint8 NodeType;					// 0 if not specified
	char *NodeDesc;					// NULL if not specified
	char *details;					// user description of node
} ExpectedNode;

#define SM_DETAILS_STRLEN 64
// additional information about a SM, from topology input
typedef struct ExpectedSM_s {
	LIST_ITEM	ExpectedSMsEntry;	// g_ExpectedSMs
	struct SMData_s *smp;			// NULL if not found
	EUI64 PortGUID;					// 0 if not specified
	EUI64 NodeGUID;					// 0 if not specified
	uint8 PortNum;					// 0-255 are valid port numbers
	uint8 gotPortNum;				// 0 if PortNum not specified
	uint8 NodeType;					// 0 if not specified
	char *NodeDesc;					// NULL if not specified
	char *details;					// user description of SM
} ExpectedSM;

#if !defined(VXWORKS) || defined(BUILD_DMC)
struct IouData_s;

typedef struct IocData_s {
	LIST_ITEM		IouIocsEntry;	// IouData.Iocs, sorted by IOC slot #
	cl_map_item_t	AllIOCsEntry;	// g_AllIOCs, key is IOC GUID
	IOC_PROFILE		IocProfile;
	uint8			IocSlot;
	struct IouData_s *ioup;	// parent IOU
	IOC_SERVICE		*Services;	// IO Services indexed by service num
	void *context;					// application specific field
} IocData;

typedef struct IouData_s {
	LIST_ITEM		AllIOUsEntry;	// g_AllIOUs
	struct NodeData_s *nodep;		// parent node
	IOUnitInfo			IouInfo;
	QUICK_LIST			Iocs;	// Iocs, IocData sorted by IOC slot #
	void *context;					// application specific field
} IouData;
#endif
// detailed information specific to an IB Switch in the fabric
typedef struct SwitchData_s {
	/**
		Lower upper-bound on number of entries allocated.

		Should be set to LinearFDBTop + 1.  Should be set to 0 when LinearFDBTop is invalid.

		Prefer LinearFDBTop & LinearFDBCap from NodeData when available.
	*/
	uint32	LinearFDBSize;
	STL_LINEAR_FORWARDING_TABLE	*LinearFDB;


	/**
		Lower upper-bound on number of entries allocated.

		Should be set to MulticastFDBTop + 1.  Should be set to 0 when 
		MulticastFDBTop is invalid.  Users are responsible for knowing where 
		the multicast address space starts.

		Prefer MulticastFDBTop & MulticastFDBCap from NodeData when available.
	*/
	uint32	MulticastFDBSize;
	uint8	MulticastFDBEntrySize;	// number of STL_PORTMASK units per entry
	STL_PORTMASK *MulticastFDB;

	/**
		Lower upper-bound on number of entries allocated.

		Should be set to PortGroupTop + 1.  Should be set to 0 when 
		PortGroupTop is invalid.  

		Prefer PortGroupTop & PortGroupCap from NodeData when available.
	*/
	uint16	PortGroupSize;
	STL_PORTMASK* PortGroupElements;
	// Always the same length as LinearFDB.
	STL_PORT_GROUP_FORWARDING_TABLE	*PortGroupFDB;

} SwitchData;
/*
	Information about a Node in the fabric.
	The SA reports a NodeRecord per port, we coallese all nodes with
	the same GUID into a single structure.
	As such the Port specific fields in the NodeInfo (PortGUID
	and LocalPortNumber) are zeroed and should not be used.
	@c NodeInfo.(Base|Class)Version should have the same values as the data
	that was used to create this record.
*/
typedef struct NodeData_s {
	cl_map_item_t	AllNodesEntry;	// g_AllNodes, key is NodeGuid
	cl_map_item_t	SystemNodesEntry;	// SystemData.Nodes, key is NodeGuid
	LIST_ITEM		AllTypesEntry;	// g_AllFIs, g_AllSWs
	struct SystemData_s *systemp;	// parent system
	STL_NODE_INFO		NodeInfo; 		// port specific fields are 0
	STL_NODE_DESCRIPTION NodeDesc;
	cl_qmap_t Ports;				// items are PortData, key is PortNum
#if !defined(VXWORKS) || defined(BUILD_DMC)
	IouData			*ioup;			// optional IOU
#endif
	STL_SWITCHINFO_RECORD *pSwitchInfo;	// optional SwitchInfo
									// also holds LID for accessing switch
									// for devices without vendor specific
									// capabilities, extra fields and capability
									// mask is zeroed
	SwitchData		*switchp;		// optional Switch specific data
	struct ExpectedNode_s *enodep;	// if supplied in topology input
	void *context;					// application specific field
	uint8	PmaAvoid:1;				// node PMA has instability
	uint8	PmaAvoidClassPortInfo:1;	// node has instability in ClassPortInfo
	uint8	PmaValidateRedirectQP:1;	// validate QP in response
	uint8	analysis:5;					// for TabulateRoutes, tier in fabric

	STL_CONGESTION_INFO CongestionInfo;
	/* CCA CongestionSetting */
	union {
		STL_SWITCH_CONGESTION_SETTING Switch;
		STL_HFI_CONGESTION_SETTING Hfi;
	} CongestionSetting;
	union {
		STL_SWITCH_CONGESTION_LOG Switch;
		STL_HFI_CONGESTION_LOG Hfi;
	} CongestionLog;
} NodeData;

typedef struct clConnPathData_s {
   IB_PATH_RECORD    path; 
} clConnPathData_t; 

typedef struct clConnData_s {
   LIST_ITEM        AllConnectionEntry; 
   uint64           FromDeviceGUID;      // GUID of the HFI, TFI, switch
   uint8            FromPortNum; 
   uint64           ToDeviceGUID;        // GUID of the HFI, TFI, switch
   uint8            ToPortNum; 
   uint32   Rate;                // active rate for this port
   clConnPathData_t PathInfo;
} clConnData_t; 

#define CREDIT_LOOP_DEVICE_MAX_CONNECTIONS      66
#define DIJKSTRA_INFINITY                       0xffff

typedef struct clDeviceData_s {
   cl_map_item_t   AllDevicesEntry;          // key is NodeGuid
   LIST_ITEM       AllDeviceTypesEntry; 
   uint16          Lid; 
   NodeData        *nodep; 
   clConnData_t    *Connections[CREDIT_LOOP_DEVICE_MAX_CONNECTIONS];       // 36 port switch + 1 for port zero
   cl_qmap_t       map_dlid_to_route;
} clDeviceData_t; 

typedef struct clRouteData_s {
   cl_map_item_t   AllRoutesEntry; // key is DLID
   PortData        *portp;
} clRouteData_t; 

typedef struct clVertixData_s {
   cl_map_item_t   AllVerticesEntry; // key is connection memory address
   clConnData_t    *Connection; 
   uint32          Id; 
   uint32          RefCount; 
   uint32          OutboundCount; 
   uint32          OutboundInuseCount; 
   uint32          OutboundLength; 
   int             *Outbound; 
   uint32          InboundCount; 
   uint32          InboundInuseCount; 
   uint32          InboundLength; 
   int             *Inbound;
} clVertixData_t; 

typedef struct clArcData_s {
   cl_map_item_t   AllArcsEntry; // key is combination of source & sink ids
   cl_map_item_t   AllArcIdsEntry; // key is arc list id
   uint32          Id; 
   uint32          Source; 
   uint32          Sink; 
   union {
      uint64	AsReg64; 
      
      struct {
         uint32	Source;
         uint32	Sink;
      } s;
   } u1;
} clArcData_t; 

typedef struct clGraphData_s {
   uint32          NumVertices; 
   uint32          NumActiveVertices; 
   uint32          VerticesLength; 
   clVertixData_t  **Vertices; 
   uint32          NumArcs; 
   //QUICK_LIST      Arcs; 
   cl_qmap_t       Arcs; 
   cl_qmap_t       map_arc_key_to_arc;
   QUICK_LIST      map_conn_to_vertex;
   cl_qmap_t       map_conn_to_vertex_conn;
} clGraphData_t; 

typedef struct clVertixDataDistance_s {
   cl_map_item_t   AllVerticesEntry;          // key is Vertix Id
   clVertixData_t  *vertixp;
} clVertixDataDistance_t; 

typedef struct clDijkstraDistancesAndRoutes_s {
    uint32          **distances;
    uint32          **routes;
    uint32          nRows;
    uint32          nCols;
} clDijkstraDistancesAndRoutes_t;

// get start of an entry (indexed by low bits of MLID) in MulticastFDB
// The entry will consist of MulticastFDBEntrySize PORTMASK values
static inline
STL_PORTMASK *GetMulticastFDBEntry(NodeData *nodep, uint32 entry)
{
	if (! nodep->switchp || ! nodep->switchp->MulticastFDB)
		return NULL;
	return (&nodep->switchp->MulticastFDB[entry * nodep->switchp->MulticastFDBEntrySize]);
}

// information about an IB enabled system in the fabric
// Each system is a set of nodes with the same SystemImageGUID
// Some 3rd party devices report a SystemImageGUID of 0, in which case
// the node GUID (which should still be unique among systems with
// SystemImageGUIDs of 0) is used as the key for g_AllSystems
typedef struct SystemData_s {
	cl_map_item_t	AllSystemsEntry;	// g_AllSystems, key is SystemImageGUID
	EUI64	SystemImageGUID;
	cl_qmap_t Nodes;					// items are NodeData, key is NodeGuid
	void *context;					// application specific field
} SystemData;

typedef struct SMData_s {
	cl_map_item_t	AllSMsEntry;			// g_AllSMs, key is PortGUID in SMInfo
	STL_SMINFO_RECORD	SMInfoRecord; // also holds LID for accessing SM
	struct PortData_s *portp;	// port SM is running on
	ExpectedSM *esmp; 			// if supplied in topology input
	void *context;					// application specific field
} SMData;

typedef struct MasterSMData_s {
	uint64	serviceID;			// Service ID of QLogic master SM (else 0)
	uint32	capabilityMask;		// Capability mask of QLogic master SM
	uint8	version;			// SM Version
} MasterSMData_t;

typedef enum {
	FF_NONE				=0,
	FF_STATS			=0x000000001,	// PortCounters fetched
	// flags for topology_input which was provided
	FF_EXPECTED_NODES	=0x000000002,	// topology_input w/<Nodes>
	FF_EXPECTED_LINKS	=0x000000004,	// topology_input w/<LinkSummary>
	FF_EXPECTED_EXTLINKS=0x000000008,	// topology_input w/<ExtLinkSummary>
	FF_CABLEDATA		=0x000000010,	// topology_input w/ some <CableData>
	FF_LIDARRAY			=0x000000020,	// Keep AllLids as array instead of qmap
										// allows faster FindLid, but takes
										// approx 300K more memory
	FF_PMADIRECT		=0x000000080,	// Force direct PMA access of port counters
	FF_ROUTES			=0x000000100,	// Routing tables (linear/mcast) fetched
	FF_QOSDATA			=0x000000200,	// QOS data fetched
	FF_SMADIRECT		=0x000000400,	// Force direct SMA access
	FF_BUFCTRLTABLE		=0x000000800,	// BufferControlData collected
	FF_DOWNPORTINFO		=0x000001000,	// Get PortInfo for Down switch ports
} FabricFlags_t;

typedef struct FabricData_s {
	time_t	time;			// when fabric data was obtained from a real fabric
	FabricFlags_t	flags;	// what data is available in FabricData

	// data from live fabric or snapshot
	cl_qmap_t AllNodes;		// items are NodeData, key is node guid
	union {
		cl_qmap_t AllLids;		// items are PortData, key is LID
		PortData **LidMap;		// allocated for max ucast lids, index is LID
	} u;
	cl_qmap_t AllSystems;	// items are SystemData, key is system image guid
	QUICK_LIST AllPorts;	// sorted by NodeGUID+PortNum
	QUICK_LIST AllFIs;		// sorted by NodeGUID
	QUICK_LIST AllSWs;		// sorted by NodeGUID
#if !defined(VXWORKS) || defined(BUILD_DMC)
	QUICK_LIST AllIOUs;		// sorted by NodeGUID
	cl_qmap_t AllIOCs;		// items are IocData, key is Ioc Guid
#endif
	cl_qmap_t AllSMs;		// items are SMData, key is PortGuid
	MasterSMData_t	MasterSMData;	// Master SM data
	uint32 LinkCount;		// number of links in fabric
	uint32 ExtLinkCount;	// number of external links in fabric

        // additional information for credit loop
        uint32 ConnectionCount;                         
        uint32 RouteCount;                         
        QUICK_LIST FIs;
        QUICK_LIST Switches;
        cl_qmap_t map_guid_to_ib_device;	// items are devices, ky is node guid
        clGraphData_t Graph;

	// additional information from topology input file
	QUICK_LIST ExpectedLinks;	// in order read from topology input file
	QUICK_LIST ExpectedFIs;		// in order read from topology input file
	QUICK_LIST ExpectedSWs;		// in order read from topology input file
	QUICK_LIST ExpectedSMs;		// in order read from topology input file
	void *context;				// application specific field
} FabricData_t;

// these callbacks are called when an object with a non-null application
// specific context field is freed.  They should free the object pointed to by
// the context field and remove any other application references to the object
// being freed
typedef void (PortDataFreeCallback)(FabricData_t *fabricp, PortData *portp);
#if !defined(VXWORKS) || defined(BUILD_DMC)
typedef void (IocDataFreeCallback)(FabricData_t *fabricp, IocData *iocp);
typedef void (IouDataFreeCallback)(FabricData_t *fabricp, IouData *ioup);
#endif
typedef void (NodeDataFreeCallback)(FabricData_t *fabricp, NodeData *nodep);
typedef void (SystemDataFreeCallback)(FabricData_t *fabricp, SystemData *systemp);
typedef void (SMDataFreeCallback)(FabricData_t *fabricp, SMData *smp);
typedef void (FabricDataFreeCallback)(FabricData_t *fabricp);

typedef struct Top_FreeCallbacks_s {
	PortDataFreeCallback	*pPortDataFreeCallback;
#if !defined(VXWORKS) || defined(BUILD_DMC)
	IocDataFreeCallback	*pIocDataFreeCallback;
	IouDataFreeCallback	*pIouDataFreeCallback;
#endif
	NodeDataFreeCallback	*pNodeDataFreeCallback;
	SystemDataFreeCallback	*pSystemDataFreeCallback;
	SMDataFreeCallback	*pSMDataFreeCallback;
	FabricDataFreeCallback	*pFabricDataFreeCallback;
} Top_FreeCallbacks;

/* struct Point_s identifies a particular point in the fabric.
 * Used for trace route and other "focused" reports
 * Presently coded in C, but a good candidate for a C++ abstract class
 * with a subclass for each point type
 */
typedef enum {
	POINT_TYPE_NONE,
	POINT_TYPE_PORT,
	POINT_TYPE_PORT_LIST,
	POINT_TYPE_NODE,
	POINT_TYPE_NODE_LIST,
#if !defined(VXWORKS) || defined(BUILD_DMC)
	POINT_TYPE_IOC,
	POINT_TYPE_IOC_LIST,
#endif
	POINT_TYPE_SYSTEM,
} PointType;
typedef struct Point_s {
	PointType	Type;
	union {
		PortData	*portp;
		NodeData	*nodep;
#if !defined(VXWORKS) || defined(BUILD_DMC)
		IocData		*iocp;
#endif
		SystemData	*systemp;
		DLIST		nodeList;
		DLIST		portList;
		DLIST		iocList;
	} u;
} Point;

#if !defined(VXWORKS) || defined(BUILD_DMC)
/* IOC types we can focus on */
typedef enum {
	IOC_TYPE_SRP,
	IOC_TYPE_OTHER
} IocType;
#endif

// used for output of snapshot
typedef struct {
	FabricData_t *fabricp;	// fabric to dump to snapshot file
	int argc;				// args to program ran
	char **argv;			// args to program ran
	// Point *focus;
} SnapshotOutputInfo_t;

// used for output of ValidateAllCreditLoopRoutes
typedef struct ValidateCreditLoopRoutesContext_s {
	uint8 format;
	int indent;
	int detail;
	int quiet;
} ValidateCreditLoopRoutesContext_t;

typedef enum {
	QUAL_EQ,
	QUAL_GE,
	QUAL_LE
} LinkQualityCompare;

// simple way to convert time_t to a localtime date string in dest
// (from Topology/getdate.c)
extern void Top_formattime(char *dest, size_t max, time_t t);

// FabricData_t handling (Topology/fabricdata.c)
extern FSTATUS InitFabricData( FabricData_t *pFabric, FabricFlags_t flags);
extern PortSelector* GetPortSelector(PortData *portp);
// Determine if portp and its neighbor are an internal link within a single
// system.  Note that some 3rd party products report SystemImageGuid of 0 in
// which case we can only consider links between the same node as internal links
extern boolean isInternalLink(PortData *portp);
// count the number of armed/active links in the node
uint32 CountInitializedPorts(FabricData_t *fabricp, NodeData *nodep);
extern FSTATUS NodeDataAllocateSwitchData(FabricData_t *fabricp, NodeData *nodep,
				uint32 LinearFDBSize, uint32 MulticastFDBSize);

/**
	Adjust amount of memory allocated for Linear and Multicast FDB tables.

	Copies existing values into new memory, adjusts FDB Top values, and frees existing memory.  When resizing MulticastFDB, the new entry size (entries per LID) must be the same as the old size.

	If the multicast FDB table is to be resized and there is already a multicast FDB table and the existing MulticastFDBTop is less than LID_MCAST_START then no values will be copied from the old table to the new table, though the table will still be resized.
	In this case, the value of @c nodep->switchp->MulticastFDBSize will be LID_MCAST_START + @c newMfdbSize rather than MulticastFDBTop + 1.

	@param newLfdbSize If 0, don't change.  Otherwise, realloc to min(newLfdbSize, LinearFDBCap).
	@param newMfdbSize If 0, don't change.  Otherwise, realloc to min(newMfdbSize, MulticastFDBCap).

	@return FSUCCESS on success, non-FSUCCESS on failure.  Existing memory is not destroyed until after new tables are successfully allocated and copied.
*/
FSTATUS NodeDataSwitchResizeFDB(NodeData * nodep, uint32 newLfdbSize, uint32 newMfdbSize);

extern FSTATUS PortDataAllocateAllQOSData(FabricData_t *fabricp);
extern FSTATUS PortDataAllocateAllBufCtrlTable(FabricData_t *fabricp);
extern FSTATUS PortDataAllocateAllPartitionTable(FabricData_t *fabricp);
extern FSTATUS PortDataAllocateAllCableInfo(FabricData_t *fabricp);
extern FSTATUS PortDataAllocateAllCongestionControlTableEntries(FabricData_t *fabricp);
extern FSTATUS PortDataAllocateAllGuidTable(FabricData_t *fabricp);

/// @param fabricp optional, can be NULL
extern void PortDataFreeQOSData(FabricData_t *fabricp, PortData *portp);
/// @param fabricp optional, can be NULL
extern FSTATUS PortDataAllocateQOSData(FabricData_t *fabricp, PortData *portp);

/// @param fabricp optional, can be NULL
extern void PortDataFreeBufCtrlTable(FabricData_t *fabricp, PortData *portp);
/// @param fabricp optional, can be NULL
extern FSTATUS PortDataAllocateBufCtrlTable(FabricData_t *fabricp, PortData *portp);

/// @param fabricp optional, can be NULL
extern void PortDataFreeCableInfoData(FabricData_t *fabricp, PortData *portp);
/// @param fabricp optional, can be NULL
extern FSTATUS PortDataAllocateCableInfoData(FabricData_t *fabricp, PortData *portp);

/// @param fabricp optional, can be NULL
extern void PortDataFreeCongestionControlTableEntries(FabricData_t *fabricp, PortData *portp);
/// @param fabricp optional, can be NULL
extern FSTATUS PortDataAllocateCongestionControlTableEntries(FabricData_t *fabricp, PortData *portp);

// these routines can be used to manually build FabricData.  Use as follows:
// for each node
// 		FabricDataAddNode (can be any state, including down)
// 		if switch node
// 			NodeDataSetSwitchInfo
// 		for each port of Node
// 			NodeDataAddPort
// 	BuildFabricDataLists
// 	for each physical link
// 		FabricDataAddLink or FabricDataAllLinkRecord
// 		(FabricDataAddLink may be called before BuildFabricDataLists if desired)
// if desired, after building the basic topology, can build empty SMA tables:
//		NodeDataAllocateAllSwitchData
//		PortDataAllocateAllQOSData
//		PortDataAllocateAllBufCtrlTable
//		PortDataAllocateAllPartitionTable
//		PortDataAllocateAllGuidTable
//	when process Set(PortInfo)
//		validate PortInfo given
//		fill in empty/noop/readonly fields with previous PortInfo
//		PortDataSetPortInfo
extern PortData* NodeDataAddPort(FabricData_t *fabricp, NodeData *nodep, EUI64 guid, STL_PORTINFO_RECORD *pPortInfo);
extern FSTATUS NodeDataSetSwitchInfo(NodeData *nodep, STL_SWITCHINFO_RECORD *pSwitchInfo);
extern NodeData *FabricDataAddNode(FabricData_t *fabricp, STL_NODE_RECORD *pNodeRecord, boolean *new_nodep);

extern boolean SupportsVendorPortCounters(NodeData *nodep,
                                        IB_CLASS_PORT_INFO *classPortInfop);
extern boolean SupportsVendorPortCounters2(NodeData *nodep,
                                        IB_CLASS_PORT_INFO *classPortInfop);

/* build the Fabric.AllPorts, ALLFIs, and AllSWs lists such that
 * AllPorts is sorted by NodeGUID, PortNum
 * AllFIs, ALLSWs, AllIOUs is sorted by NodeGUID
 */
extern void BuildFabricDataLists(FabricData_t *fabricp);

extern FSTATUS FabricDataAddLink(FabricData_t *fabricp, PortData *p1, PortData *p2);
extern FSTATUS FabricDataAddLinkRecord(FabricData_t *fabricp, STL_LINK_RECORD *pLinkRecord);
extern FSTATUS FabricDataRemoveLink(FabricData_t *fabricp, PortData *p1);
// Set update lookup tables due to PortState, LID or LMC change for port.
// Typical use is when a SetPortInfo on neighbor causes a link state change
// For use by fabric simulator.
extern void PortDataStateChanged(FabricData_t *fabricp, PortData *portp);
// Set new Port Info based on a Set(PortInfo).  For use by fabric simulator
// assumes pInfo already validated and any Noop fields filled in with correct
// values.
extern void PortDataSetPortInfo(FabricData_t *fabricp, PortData *portp, STL_PORT_INFO *pInfo);

extern void DestroyFabricData(FabricData_t *fabricp);

extern NodeData * CLDataAddDevice(FabricData_t *fabricp, NodeData *nodep, uint16 lid, int verbose);
extern FSTATUS CLDataAddConnection(FabricData_t *fabricp, PortData *portp1, PortData *portp2, clConnPathData_t *pathInfo, int verbose);
extern FSTATUS CLDataAddRoute(FabricData_t *fabricp, uint16 slid, uint16 dlid, PortData *sportp, int verbose);
extern clGraphData_t * CLGraphDataSplit(clGraphData_t *graphp, int verbose);
extern FSTATUS CLFabricDataDestroy(FabricData_t *fabricp, void *context);
extern FSTATUS CLDijkstraFindDistancesAndRoutes(clGraphData_t *graphp, clDijkstraDistancesAndRoutes_t *respData, int verbose);
extern void CLDijkstraFreeDistancesAndRoutes(clDijkstraDistancesAndRoutes_t *drp);

// search routines (Topology/search.c)
// search for the PortData corresponding to the given node and port number
extern PortData * FindNodePort(NodeData *nodep, uint8 port);
// search for the PortData corresponding to the given lid
extern PortData * FindLid(FabricData_t* fabricp, uint16 lid);
// search for the PortData corresponding to the given port Guid
extern PortData * FindPortGuid(FabricData_t* fabricp, EUI64 guid);
// search for the NodeData corresponding to the given node Guid
extern NodeData * FindNodeGuid(const FabricData_t* fabricp, EUI64 guid);
extern FSTATUS FindNodeName(FabricData_t* fabricp, char *name, Point *pPoint, int silent);
extern FSTATUS FindNodeNamePat(FabricData_t* fabricp, char *pattern, Point *pPoint);
extern FSTATUS FindNodeDetailsPat(FabricData_t* fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindNodeType(FabricData_t* fabricp, NODE_TYPE type, Point *pPoint);
#if !defined(VXWORKS) || defined(BUILD_DMC)
extern FSTATUS FindIocName(FabricData_t* fabricp, char *name, Point *pPoint);
extern FSTATUS FindIocNamePat(FabricData_t* fabricp, char *pattern, Point *pPoint);
extern FSTATUS FindIocType(FabricData_t* fabricp, IocType type, Point *pPoint);
extern IocData * FindIocGuid(FabricData_t* fabricp, EUI64 guid);
#endif
extern SystemData * FindSystemGuid(FabricData_t* fabricp, EUI64 guid);
extern FSTATUS FindRate(FabricData_t* fabricp, uint32 rate, Point *pPoint);
extern FSTATUS FindPortState(FabricData_t* fabricp, uint8 state, Point *pPoint);
extern FSTATUS FindLedState(FabricData_t* fabricp, uint8 state, Point *pPoint);
extern FSTATUS FindPortPhysState(FabricData_t* fabricp, IB_PORT_PHYS_STATE state, Point *pPoint);
extern FSTATUS FindCableLabelPat(FabricData_t* fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCableLenPat(FabricData_t* fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCableDetailsPat(FabricData_t* fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCabinfLenPat(FabricData_t *fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCabinfVendNamePat(FabricData_t *fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCabinfVendPNPat(FabricData_t *fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCabinfVendRevPat(FabricData_t *fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCabinfVendSNPat(FabricData_t *fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindCabinfCableType(FabricData_t *fabricp, char *cablearg, Point *pPoint);
extern FSTATUS FindLinkDetailsPat(FabricData_t* fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindPortDetailsPat(FabricData_t* fabricp, const char* pattern, Point *pPoint);
extern FSTATUS FindMtu(FabricData_t* fabricp, IB_MTU mtu, uint8 vl_num, Point *pPoint);
extern PortData * FindMasterSm(FabricData_t* fabricp);
extern FSTATUS FindSmDetailsPat(FabricData_t *fabricp,const char* pattern, Point *pPoint);
// search for the SMData corresponding to the given PortGuid
extern SMData * FindSMPort(FabricData_t *fabricp,EUI64 PortGUID);
// search for the PortData corresponding to the given lid and port number
// For FIs lid completely defines the port
// For Switches, lid will identify the switch and port is used to select port
extern PortData * FindLidPort(FabricData_t *fabricp,uint16 lid, uint8 port);
extern PortData * FindNodeGuidPort(FabricData_t *fabricp,EUI64 nodeguid, uint8 port);
extern ExpectedNode* FindExpectedNodeByNodeGuid(FabricData_t* fabricp, EUI64 nodeGuid);
extern ExpectedLink* FindExpectedLinkByOneSide(FabricData_t* fabricp, EUI64 nodeGuid, uint8 portNum, uint8* side);
extern ExpectedLink* FindExpectedLinkByNodeGuid(FabricData_t* fabricp, EUI64 nodeGuid1, EUI64 nodeGuid2);
extern ExpectedLink* FindExpectedLinkByPortGuid(FabricData_t* fabricp, EUI64 portGuid1, EUI64 portGuid2);
extern ExpectedLink* FindExpectedLinkByNodeDesc(FabricData_t* fabricp, char* nodeDesc1, char* nodeDesc2);
extern FSTATUS FindLinkQuality(FabricData_t *fabricp, uint16 quality, LinkQualityCompare comp, Point *pPoint);

// mad queries to PMA and DMA (from Topology/mad.c)
extern FSTATUS InitMad(EUI64 portguid, FILE *verbose_file);
extern void DestroyMad(void);
extern FSTATUS InitSmaMkey(uint64 mkey);
extern boolean NodeHasPma(NodeData *nodep);
extern boolean PortHasPma(PortData *portp);
extern void UpdateNodePmaCapabilities(NodeData *nodep, boolean ProcessHFICounters);
extern FSTATUS PmGetClassPortInfo(struct oib_port *port, PortData *portp);
extern FSTATUS PmGetPortCounters(struct oib_port *port, PortData *portp, uint8 portNum,
							PORT_COUNTERS *pPortCounters);
extern FSTATUS STLPmGetClassPortInfo(struct oib_port *port, PortData *portp);
extern FSTATUS STLPmGetPortStatus(struct oib_port *port, PortData *portp, uint8 portNum, STL_PortStatusData_t *pPortStatus);
extern FSTATUS STLPmClearPortCounters(struct oib_port *port, PortData *portp, uint8 lastPortIndex, uint32 counterselect);
#if !defined(VXWORKS) || defined(BUILD_DMC)
extern FSTATUS DmGetIouInfo(struct oib_port *port, IB_PATH_RECORD *pathp, IOUnitInfo *pIouInfo);
extern FSTATUS DmGetIocProfile(struct oib_port *port, IB_PATH_RECORD *pathp, uint8 slot,
						IOC_PROFILE *pIocProfile);
extern FSTATUS DmGetServiceEntries(struct oib_port *port, IB_PATH_RECORD *pathp, uint8 slot,
							uint8 first, uint8 last, IOC_SERVICE *pIocServices);
#endif

// POINT routines (from Topology/point.c)
extern void PointInit(Point *point);
extern void PointDestroy(Point *point);
extern FSTATUS PointCopy(Point *dest, Point *src);
extern FSTATUS PointListAppend(Point *point, PointType type, void *object);
extern void PointCompress(Point *point);
extern FSTATUS PointListAppendUniquePort(Point *point, PortData *portp);
/* compare the supplied port to the given point
 * this is used to identify focus for reports
 * POINT_TYPE_NONE will report TRUE for all ports
 */
extern boolean ComparePortPoint(PortData *portp, Point *point);
/* compare the supplied node to the given point
 * this is used to identify focus for reports
 * POINT_TYPE_NONE will report TRUE for all nodes
 */
extern boolean CompareNodePoint(NodeData *nodep, Point *point);
#if !defined(VXWORKS) || defined(BUILD_DMC)
extern boolean CompareIouPoint(IouData *ioup, Point *point);
extern boolean CompareIocPoint(IocData *iocp, Point *point);
#endif
/* compare the supplied SM to the given point
 * this is used to identify focus for reports
 * POINT_TYPE_NONE will report TRUE for all SMs
 */
extern boolean CompareSmPoint(SMData *smp, Point *point);
extern boolean CompareSystemPoint(SystemData *systemp, Point *point);

/* check arg to see if 1st characters match prefix, if so return pointer
 * into arg just after prefix, otherwise return NULL
 */
extern char* ComparePrefix(char *arg, const char *prefix);
extern FSTATUS ParsePoint(FabricData_t *fabricp, char* arg, Point* pPoint, char **pp);

// snapshot input/output routines (from Topology/snapshot.c)
extern void Xml2PrintSnapshot(FILE *file, SnapshotOutputInfo_t *info);

struct IXmlParserState;

/**
	Completion callback manipulators.  These get called inside custom end_func
	definitions and typically do things like insert the completed
	structure into a broader data structure.

	The basic idea is to separate how the entity is parsed from how it is used by the parent.
*/
typedef int (*ParseCompleteFn)(struct IXmlParserState * state, void * object, void * parent);

void SetPortDataComplete(ParseCompleteFn fn);

/**
	only FF_LIDARRAY flag is used, others set based on file read
	@param allocFull When true, adjust allocated memory for linear and multicast forwarding tables to their cap values after reading in all data.
*/
#ifndef __VXWORKS__
extern FSTATUS Xml2ParseSnapshot(const char *input_file, int quiet, FabricData_t *fabricp, FabricFlags_t flags, boolean allocFull);
#else
extern FSTATUS Xml2ParseSnapshot(const char *input_file, int quiet, FabricData_t *fabricp, FabricFlags_t flags, boolean allocFull, XML_Memory_Handling_Suite* memsuite);
#endif
 
// expected topology input/output routines (from Topology/topology.c)
#ifndef __VXWORKS__
extern FSTATUS Xml2ParseTopology(const char *input_file, int quiet, FabricData_t *fabricp);
#else
extern FSTATUS Xml2ParseTopology(const char *input_file, int quiet, FabricData_t *fabricp, XML_Memory_Handling_Suite* memsuite);
#endif
extern void Xml2PrintTopology(FILE *file, FabricData_t *fabricp);

// live fabric analysis routines (from Topology/sweep.c)
extern FSTATUS InitSweepVerbose(FILE *verbose_file);
/* flags for Sweep */
typedef enum {
	SWEEP_BASIC					=0,		// Systems, Nodes, Ports, Links
#if !defined(VXWORKS) || defined(BUILD_DMC)
	SWEEP_IOUS			=0x000000001,	// IOU and IOC Info
#endif
	SWEEP_SWITCHINFO	=0x000000002,	// Switch Info
	SWEEP_SM			=0x000000003,	// SM Info
	SWEEP_ALL			=0x000000003
} SweepFlags_t;

extern FSTATUS Sweep(EUI64 portGuid, FabricData_t *fabricp, FabricFlags_t fflags, SweepFlags_t flags, int quiet);

//extern FSTATUS GetPathToPort(EUI64 portGuid, PortData *portp, uint16 pkey);
extern FSTATUS GetPaths(struct oib_port *port, PortData *portp1, PortData *portp2,
						PQUERY_RESULT_VALUES *ppQueryResults);
extern FSTATUS GetTraceRoute(struct oib_port *port, IB_PATH_RECORD *pathp,
							 PQUERY_RESULT_VALUES *ppQueryResults);
extern FSTATUS GetAllPortCounters(EUI64 portGuid, IB_GID localGid, FabricData_t *fabricp,
			   	Point *focus, boolean limitstats, boolean quiet, uint32 begin, uint32 end);
extern FSTATUS GetAllFDBs( EUI64 portGuid, FabricData_t *fabricp, Point *focus,
				int quiet );
extern FSTATUS GetAllPortVLInfo(EUI64 portGuid, FabricData_t *fabricp, Point *focus, int quiet);
extern PQUERY_RESULT_VALUES GetAllVFInfo(struct oib_port *port, FabricData_t *fabricp, 
										 Point *focus, int quiet);
extern PQUERY_RESULT_VALUES GetAllQuarantinedNodes(struct oib_port *port, FabricData_t *fabricp,
													Point *focus, int quiet);
extern FSTATUS GetAllBCTs(EUI64 portGuid, FabricData_t *fabricp, Point *focus, int quiet);

// port 0 gets a bit as well as ports 1-NumPorts
// this returns Entry size in sizeof(PORTMASK) units
static __inline uint32 ComputeMulticastFDBEntrySize(uint8 numports)
{
	return ROUNDUP( (numports + 1),
			(sizeof(STL_PORTMASK) * 8) ) / (sizeof(STL_PORTMASK) * 8);
}

extern FSTATUS ClearAllPortCounters(EUI64 portGuid, IB_GID localGid, FabricData_t *fabricp,
			  	Point *focus, uint32 counterselect, boolean limitstats,
			   	boolean quiet, uint32 *node_countp, uint32 *port_countp,
			   	uint32 *fail_node_countp, uint32 *fail_port_countp);

extern void XmlPrintHex64(const char *tag, uint64 value, int indent);
extern FSTATUS ParseFocusPoint(EUI64 portGuid, FabricData_t *fabricp, char* arg, Point* pPoint, char **pp, boolean allow_route);

// utility routines (from Topology/util.c)
extern void Top_setcmdname(const char *name);	// for error messages
extern void Top_setFreeCallbacks(Top_FreeCallbacks *callbacks);
extern void ProgressPrint(boolean newline, const char *format, ...);
extern const char* Top_truncate_str(const char *name);
//#define PROGRESS_PRINT(newline, format, args...) if (! g_quiet) { ProgressPrint(newline, format, ##args); } 


// functions to perform routing analysis using LFT tables in FabricData
//(from Topology/route.c)

// For each device along the path, entry and exit port of device is provided
// For the CA at the start of the route, only an exit port is provided
// For the CA at the end of the route, only a entry port is provided
// For switches along the route, both entry and exit ports are provided
// When a switch Port 0 is the start of the route, it will be the entry port
//    along with the physical exit port
// When a switch Port 0 is the end of the route, it will be the exit port
//    along with the physical entry port
// The above approach parallels how TraceRoute records are reported by the
// SM, so if desired a callback could buil a SM TraceRoute style response
// for use in other routines.
typedef FSTATUS (RouteCallback_t)(PortData *entryPortp, PortData *exitPortp,
			   						void *context);

// returns status of callback (if not FSUCCESS)
// FUNAVAILABLE - no routing tables in FabricData given
// FNOT_FOUND - unable to find starting port
// FNOT_DONE - unable to trace route, dlid is a dead end
extern FSTATUS WalkRoutePort(FabricData_t *fabricp,
			   		PortData *portp, IB_LID dlid,
			  		RouteCallback_t *callback, void *context);
// walk by slid to dlid
extern FSTATUS WalkRoute(FabricData_t *fabricp, IB_LID slid, IB_LID dlid,
			  		RouteCallback_t *callback, void *context);

// caller must free *ppTraceRecords
extern FSTATUS GenTraceRoutePort(FabricData_t *fabricp,
			   	PortData *portp, IB_LID dlid,
	   			STL_TRACE_RECORD **ppTraceRecords, uint32 *pNumTraceRecords);
extern FSTATUS GenTraceRoute(FabricData_t *fabricp, IB_LID slid, IB_LID dlid,
	   			STL_TRACE_RECORD **ppTraceRecords, uint32 *pNumTraceRecords);
extern FSTATUS GenTraceRoutePath(FabricData_t *fabricp, IB_PATH_RECORD *pathp,
	   			STL_TRACE_RECORD **ppTraceRecords, uint32 *pNumTraceRecords);

// Generate possible Path records from portp1 to portp2
// We don't know SM config, so we just guess and generate paths of the form
// 0-0, 1-1, ....
// This corresponds to the PathSelection=Minimal FM config option
// when LMC doesn't match we start back at base lid for other port
// These may not be accurate for Torus, however the dlid is all that really
// matters for route analysis, so this should be fine
extern FSTATUS GenPaths(FabricData_t *fabricp,
		PortData *portp1, PortData *portp2,
		IB_PATH_RECORD **ppPathRecords, uint32 *pNumPathRecords);

// tabulate all the ports along the route from slid to dlid
extern FSTATUS TabulateRoute(FabricData_t *fabricp, IB_LID slid, IB_LID dlid,
					boolean fatTree);
// tabulate all routes from portp1 to portp2
extern FSTATUS TabulateRoutes(FabricData_t *fabricp,
			   		PortData *portp1, PortData *portp2, uint32 *totalPaths,
					uint32 *badPaths, boolean fatTree);
// tabulate all the routes between FIs, exclude loopback routes
extern FSTATUS TabulateCARoutes(FabricData_t *fabricp, uint32 *totalPaths,
					uint32 *badPaths, boolean fatTree);

typedef void (*ReportCallback_t)(PortData *portp1, PortData *portp2,
			   		IB_LID dlid, boolean isBaseLid,
				   	boolean flag /* TRUE=uplink or Recv */, void *context);

// report all routes from portp1 to portp2 that cross reportPort
extern FSTATUS ReportRoutes(FabricData_t *fabricp,
			   		PortData *portp1, PortData *portp2,
				   	PortData *reportPort,
				   	ReportCallback_t callback, void *context, boolean fatTree);
// report all the routes between FIs that cross reportPort,
// exclude loopback routes
extern FSTATUS ReportCARoutes(FabricData_t *fabricp,
			   		PortData *reportPort,
				   	ReportCallback_t callback, void *context, boolean fatTree);

// callback used to indicate that an incomplete route exists between p1 and p2
typedef void (*ValidateCallback_t)(PortData *portp1, PortData *portp2,
			   		IB_LID dlid, boolean isBaseLid, void *context);

// callback used to indicate a port along an incomplete route
typedef void (*ValidateCallback2_t)(PortData *portp, void *context);

// validate all routes from portp1 to portp2
extern FSTATUS ValidateRoutes(FabricData_t *fabricp,
			   		PortData *portp1, PortData *portp2,
					uint32 *totalPaths, uint32 *badPaths,
				   	ValidateCallback_t callback, void *context,
				   	ValidateCallback2_t callback2, void *context2);
// validate all the routes between all LIDs
// exclude loopback routes
extern FSTATUS ValidateAllRoutes(FabricData_t *fabricp,
					uint32 *totalPaths, uint32 *badPaths,
				   	ValidateCallback_t callback, void *context,
				   	ValidateCallback2_t callback2, void *context2);

typedef void (*ValidateCLRouteCallback_t)(PortData *portp1, PortData *portp2, void *context); 

typedef void (*ValidateCLFabricSummaryCallback_t)(FabricData_t *fabricp, const char *name,
						  uint32 totalPaths, uint32 totalBadPaths,
						  void *context);
typedef void (*ValidateCLDataSummaryCallback_t)(clGraphData_t *graphp, const char *name, void *context);
typedef void (*ValidateCLRouteSummaryCallback_t)(uint32 routesPresent, 
                                                 uint32 routesMissing, 
                                                 uint32 hopsHistogramEntries, 
                                                 uint32 *hopsHistogram, 
                                                 void *context); 
typedef void (*ValidateCLPathSummaryCallback_t)(FabricData_t *fabricp, clConnData_t *connp, int indent, void *context);
typedef void (*ValidateCLLinkSummaryCallback_t)(uint32 id, const char *name, uint32 cycle, uint8 header, int indent, void *context);
typedef void (*ValidateCLLinkStepSummaryCallback_t)(uint32 id, const char *name, uint32 step, uint8 header, int indent, void *context); 

#ifndef __VXWORKS__
typedef FSTATUS (*ValidateCLTimeGetCallback_t)(uint64_t *address, pthread_mutex_t *lock);

extern pthread_mutex_t g_cl_lock; 
extern FSTATUS ValidateAllCreditLoopRoutes(FabricData_t *fabricp, EUI64 portGuid, 
                                           ValidateCLRouteCallback_t routeCallback,
                                           ValidateCLFabricSummaryCallback_t fabricSummaryCallback,
                                           ValidateCLDataSummaryCallback_t dataSummaryCallback,
                                           ValidateCLRouteSummaryCallback_t routeSummaryCallback,
                                           ValidateCLLinkSummaryCallback_t linkSummaryCallback,
                                           ValidateCLLinkStepSummaryCallback_t linkStepSummaryCallback,
                                           ValidateCLPathSummaryCallback_t pathSummaryCallback,
                                           ValidateCLTimeGetCallback_t timeGetCallback,
                                           void *context, 
                                           uint8 snapshotInFile); 
extern void CLFabricSummary(FabricData_t *fabricp, const char *name, ValidateCLFabricSummaryCallback_t callback,
			    uint32 totalPaths, uint32 totalBadPaths, void *context);
extern void CLGraphDataSummary(clGraphData_t *graphp, const char *name, ValidateCLDataSummaryCallback_t callback, void *context);
extern FSTATUS CLFabricDataBuildRouteGraph(FabricData_t *fabricp,
                                           ValidateCLRouteSummaryCallback_t routeSummaryCallback,
                                           ValidateCLTimeGetCallback_t timeGetCallback,
                                           void *context);
extern void CLGraphDataPrune(clGraphData_t *graphp, ValidateCLTimeGetCallback_t timeGetCallback, int verbose);
extern void CLDijkstraFindCycles(FabricData_t *fabricp,
                                 clGraphData_t *graphp,
                                 clDijkstraDistancesAndRoutes_t *drp,
                                 ValidateCLLinkSummaryCallback_t linkSummaryCallback,
                                 ValidateCLLinkStepSummaryCallback_t linkStepSummaryCallback,
                                 ValidateCLPathSummaryCallback_t pathSummaryCallback,
                                 void *context);
extern FSTATUS CLTimeGet(uint64_t *address); 
#endif

static __inline uint32 ComputeMulticastFDBSize(const STL_SWITCH_INFO *pSwitchInfo)
{
	//STL Volg1 20.2.2.6.5
	return (pSwitchInfo->MulticastFDBTop >= LID_MCAST_START) ? 
			pSwitchInfo->MulticastFDBTop-LID_MCAST_START+1 : 0;
}

#ifdef __cplusplus
};
#endif

#endif /* _TOPOLOGY_H */
