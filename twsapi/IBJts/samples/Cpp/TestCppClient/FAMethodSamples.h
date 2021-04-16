﻿/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_SAMPLES_TESTCPPCLIENT_FAMETHODSAMPLES_H
#define TWS_API_SAMPLES_TESTCPPCLIENT_FAMETHODSAMPLES_H

#include <string>

class FAMethodSamples {
public:
	static std::string FAOneGroup(){
		//! [faonegroup]
		std::string groupXml = 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<ListOfGroups>"
			"<Group>"
				"<name>Equal_Quantity</name>"
				"<ListOfAccts varName=\"list\">"
					"<String>DU119915</String>"
					"<String>DU119916</String>"
				"</ListOfAccts>"
				"<defaultMethod>EqualQuantity</defaultMethod>"
			"</Group>"
		"</ListOfGroups>";
		//! [faonegroup]
		return groupXml;
	}

	static std::string FATwoGroups(){
		//! [fatwogroups]
		std::string groupXml = 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<ListOfGroups>"
			"<Group>"
				"<name>Equal_Quantity</name>"
				"<ListOfAccts varName=\"list\">"
					"<String>DU119915</String>"
					"<String>DU119916</String>"
				"</ListOfAccts>"
				"<defaultMethod>EqualQuantity</defaultMethod>"
			"</Group>"
			"<Group>"
				"<name>Pct_Change</name>"
				"<ListOfAccts varName=\"list\">"
					"<String>DU119915</String>"
					"<String>DU119916</String>"
				"</ListOfAccts>"
				"<defaultMethod>PctChange</defaultMethod>"
			"</Group>"
		"</ListOfGroups>";
		//! [fatwogroups]
		return groupXml;
	}

	static std::string FAOneProfile(){
		//! [faoneprofile]
		std::string profileXml = 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<ListOfAllocationProfiles>"
			"<AllocationProfile>"
				"<name>Percent_60_40</name>"
				"<type>1</type>"
				"<ListOfAllocations varName=\"listOfAllocations\">"
					"<Allocation>"
						"<acct>DU119915</acct>"
						"<amount>60.0</amount>"
					"</Allocation>"
					"<Allocation>"
						"<acct>DU119916</acct>"
						"<amount>40.0</amount>"
					"</Allocation>"
				"</ListOfAllocations>"
			"</AllocationProfile>"
		"</ListOfAllocationProfiles>";
		//! [faoneprofile]
		return profileXml;
	}

	static std::string FATwoProfiles(){
		//! [fatwoprofiles]
		std::string profileXml = 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<ListOfAllocationProfiles>"
			"<AllocationProfile>"
				"<name>Percent_60_40</name>"
				"<type>1</type>"
				"<ListOfAllocations varName=\"listOfAllocations\">"
					"<Allocation>"
						"<acct>DU119915</acct>"
						"<amount>60.0</amount>"
					"</Allocation>"
					"<Allocation>"
						"<acct>DU119916</acct>"
						"<amount>40.0</amount>"
					"</Allocation>"
				"</ListOfAllocations>"
			"</AllocationProfile>"
			"<AllocationProfile>"
				"<name>Ratios_2_1</name>"
				"<type>1</type>"
				"<ListOfAllocations varName=\"listOfAllocations\">"
					"<Allocation>"
						"<acct>DU119915</acct>"
						"<amount>2.0</amount>"
					"</Allocation>"
					"<Allocation>"
						"<acct>DU119916</acct>"
						"<amount>1.0</amount>"
					"</Allocation>"
				"</ListOfAllocations>"
			"</AllocationProfile>"
		"</ListOfAllocationProfiles>";
		//! [fatwoprofiles]
		return profileXml;
	}

};

#endif