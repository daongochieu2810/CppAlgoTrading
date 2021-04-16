﻿/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_COMMISSIONREPORT_H
#define TWS_API_CLIENT_COMMISSIONREPORT_H

#include <string>

struct CommissionReport
{
	CommissionReport()
	{
		commission = 0;
		realizedPNL = 0;
		yield = 0;
		yieldRedemptionDate = 0;
	}

	// commission report fields
	std::string	execId;
	double		commission;
	std::string	currency;
	double		realizedPNL;
	double		yield;
	int			yieldRedemptionDate; // YYYYMMDD format
};

#endif // commissionreport_def
