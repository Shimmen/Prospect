#pragma once

#include <memory>

#include "App.h"

// Include apps below //
#include "TestApp.h"
#include "IBLDemo.h"
#include "PointcloudExplorer.h"
////////////////////////

namespace AppSelector
{
	std::unique_ptr<App> ConstructApp()
	{
		// Change to whatever app you want to run
		return std::make_unique<IBLDemo>();
	}
}
