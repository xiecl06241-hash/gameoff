#include "ShiftTypes.h"

namespace ShiftData
{
	const TArray<FShiftProduct>& Products()
	{
		// Costs mirror SourceArt/products.json. Price is cost x1.6; stun scales with weight/size.
		static const TArray<FShiftProduct> List = {
			{ TEXT("Water"),    TEXT("PURE Spring Water"),     2.0f, 3.2f,  2.5f },
			{ TEXT("Milk"),     TEXT("DAILY Whole Milk"),      3.4f, 5.4f,  3.0f },
			{ TEXT("Soda"),     TEXT("CITRA Sparkling Soda"),  2.8f, 4.5f,  2.5f },
			{ TEXT("Noodles"),  TEXT("MIDNIGHT Noodle Cup"),   4.0f, 6.4f,  1.5f },
			{ TEXT("Chips"),    TEXT("CRISP Potato Chips"),    3.2f, 5.1f,  1.0f },
			{ TEXT("Biscuits"), TEXT("OAT Biscuits"),          4.2f, 6.7f,  1.5f },
			{ TEXT("Can"),      TEXT("PANTRY Tomato Soup"),    3.5f, 5.6f,  4.0f },
			{ TEXT("Bread"),    TEXT("MORNING Bread"),         2.4f, 3.8f,  1.0f },
			{ TEXT("Tissue"),   TEXT("SOFT Tissues"),          2.1f, 3.4f,  0.8f },
			{ TEXT("Meal"),     TEXT("FRESH Rice Bowl"),       7.0f, 11.2f, 2.0f },
			{ TEXT("Sauce"),    TEXT("UMAMI Soy Sauce"),       4.5f, 7.2f,  3.5f },
			{ TEXT("Cleaner"),  TEXT("CLEAR Surface Cleaner"), 5.5f, 8.8f,  3.5f },
		};
		return List;
	}

	const FShiftProduct* FindProduct(FName Sku)
	{
		return Products().FindByPredicate([Sku](const FShiftProduct& P) { return P.Sku == Sku; });
	}

	FString ProductLabel(FName Sku)
	{
		const FShiftProduct* P = FindProduct(Sku);
		return P ? FString(P->Label) : Sku.ToString();
	}

	FName CounterSkuForTell(EDisguiseTell Tell)
	{
		switch (Tell)
		{
		case EDisguiseTell::HeadTilt: return TEXT("Can");
		case EDisguiseTell::RedGlare: return TEXT("Milk");
		case EDisguiseTell::Floating: return TEXT("Bread");
		default: return TEXT("Can");
		}
	}

	FName ShiftedSku(FName NormalSku)
	{
		// During a store Shift, cold storage and dry shelves swap stock categories.
		if (NormalSku == TEXT("Water"))    { return TEXT("Can"); }
		if (NormalSku == TEXT("Can"))      { return TEXT("Water"); }
		if (NormalSku == TEXT("Milk"))     { return TEXT("Sauce"); }
		if (NormalSku == TEXT("Sauce"))    { return TEXT("Milk"); }
		if (NormalSku == TEXT("Soda"))     { return TEXT("Bread"); }
		if (NormalSku == TEXT("Bread"))    { return TEXT("Soda"); }
		if (NormalSku == TEXT("Noodles"))  { return TEXT("Meal"); }
		if (NormalSku == TEXT("Meal"))     { return TEXT("Noodles"); }
		if (NormalSku == TEXT("Chips"))    { return TEXT("Biscuits"); }
		if (NormalSku == TEXT("Biscuits")) { return TEXT("Chips"); }
		if (NormalSku == TEXT("Tissue"))   { return TEXT("Cleaner"); }
		if (NormalSku == TEXT("Cleaner"))  { return TEXT("Tissue"); }
		return NormalSku;
	}

	const TArray<FShiftTaskSpot>& AnomalySpots()
	{
		static const TArray<FShiftTaskSpot> List = {
			/*0*/ { EShiftTaskType::FixAnomaly, FVector(1080.f, 940.f, 120.f), TEXT("Reset warped welcome sign") },
			/*1*/ { EShiftTaskType::FixAnomaly, FVector(800.f, 450.f, 110.f),  TEXT("Push shifted Aisle 02 gondola back") },
			/*2*/ { EShiftTaskType::FixAnomaly, FVector(350.f, 925.f, 120.f),  TEXT("Close rattling cooler & check stock") },
		};
		return List;
	}

	const TArray<FShiftTaskSpot>& TaskSpots()
	{
		static const TArray<FShiftTaskSpot> List = {
			/*0*/ { EShiftTaskType::Restock, FVector(400.f, 515.f, 110.f),  TEXT("Restock Aisle 01") },
			/*1*/ { EShiftTaskType::Restock, FVector(1200.f, 685.f, 110.f), TEXT("Restock Aisle 03") },
			/*2*/ { EShiftTaskType::Restock, FVector(1515.f, 620.f, 110.f), TEXT("Restock wall shelf") },
			/*3*/ { EShiftTaskType::Count,   FVector(350.f, 925.f, 120.f),  TEXT("Count cold storage") },
			/*4*/ { EShiftTaskType::Count,   FVector(400.f, 1440.f, 120.f), TEXT("Count warehouse stock") },
			/*5*/ { EShiftTaskType::Tidy,    FVector(800.f, 392.f, 110.f),  TEXT("Tidy Aisle 02") },
			/*6*/ { EShiftTaskType::Tidy,    FVector(135.f, 470.f, 120.f),  TEXT("Tidy drinks case") },
			/*7*/ { EShiftTaskType::Trash,   FVector(1500.f, 265.f, 80.f),  TEXT("Empty checkout bin") },
			/*8*/ { EShiftTaskType::Trash,   FVector(1350.f, 1450.f, 80.f), TEXT("Empty restroom bin") },
			/*9*/ { EShiftTaskType::Count,   FVector(1070.f, 1450.f, 110.f),TEXT("Sign the shift log") },
			/*10*/{ EShiftTaskType::CookOden,FVector(1380.f, 180.f, 220.f), TEXT("Prepare hot oden") },
		};
		return List;
	}

	const TArray<FVector>& BrowsePoints()
	{
		static const TArray<FVector> List = {
			FVector(400.f, 600.f, 100.f), FVector(1200.f, 600.f, 100.f), FVector(800.f, 900.f, 100.f),
			FVector(350.f, 900.f, 100.f), FVector(1450.f, 620.f, 100.f), FVector(200.f, 470.f, 100.f),
		};
		return List;
	}

	const TArray<FVector>& LostItemSpots()
	{
		static const TArray<FVector> List = {
			FVector(1120.f, 1480.f, 90.f), FVector(1500.f, 1400.f, 10.f), FVector(620.f, 1350.f, 10.f),
			FVector(120.f, 1125.f, 10.f),  FVector(1420.f, 110.f, 10.f),  FVector(520.f, 925.f, 10.f),
		};
		return List;
	}

	const TArray<FVector>& RestockSpots()
	{
		static const TArray<FVector> List = { TaskSpots()[0].Location, TaskSpots()[1].Location, TaskSpots()[2].Location };
		return List;
	}

	const TArray<FShiftNightSpec>& Nights()
	{
		static TArray<FShiftNightSpec> List;
		if (List.Num() == 0)
		{
			auto Customer = [](const TCHAR* Sku, bool bHelp, const TCHAR* Line = TEXT("")) {
				FShiftCustomerSpec C; C.WantedSku = Sku; C.bAsksHelp = bHelp; C.Line = Line; return C;
			};
			auto Monster = [](const TCHAR* Sku, const TCHAR* Lost, const TCHAR* Line, const TCHAR* Hint) {
				FShiftCustomerSpec C; C.WantedSku = Sku; C.bAsksHelp = true; C.bMonster = true; C.MonsterSku = Lost; C.Line = Line; C.AnomalyHint = Hint;
				if (FName(Lost) == TEXT("Can"))   { C.Tell = EDisguiseTell::HeadTilt; }
				else if (FName(Lost) == TEXT("Milk"))  { C.Tell = EDisguiseTell::RedGlare; }
				else if (FName(Lost) == TEXT("Bread")) { C.Tell = EDisguiseTell::Floating; }
				else { C.Tell = EDisguiseTell::HeadTilt; }
				return C;
			};

			FShiftNightSpec N1;
			N1.Title = TEXT("NIGHT 1");
			N1.Intro = TEXT("02:00 AM. You took over the late shift at this corner store.\nRumor says a murder happened here years ago, though nobody talks about what happened.\nCheck your pocket note [Tab] for tonight's rule, refuse anyone abnormal [R], and press [Q] to glimpse the old scene if the store feels wrong.");
			N1.Customers = { Customer(TEXT("Water"), false), Customer(TEXT("Noodles"), true, TEXT("Excuse me... where are the noodle cups?")), Customer(TEXT("Chips"), false) };
			N1.OpeningTasks = { 3, 5 };
			N1.Spills = 2; N1.TaskTimeLimit = 240.f; N1.bEerieEvents = true;
			List.Add(N1);

			FShiftNightSpec N2;
			N2.Title = TEXT("NIGHT 2");
			N2.Intro = TEXT("A second rule was scribbled onto the counter note before you arrived.\nSome visitors wear familiar faces, but their habits give them away.\nCheck [Tab] before handing over goods — or refuse them with [R].");
			N2.Customers = { Customer(TEXT("Milk"), false), Customer(TEXT("Bread"), true, TEXT("Do you have any bread left?")),
				Monster(TEXT("Soda"), TEXT("Can"), TEXT("I... need something to drink."), TEXT("...So thirsty for thick red tomato soup in a tin can...")), Customer(TEXT("Meal"), false) };
			N2.OpeningTasks = { 0, 4 };
			N2.Spills = 3; N2.TaskTimeLimit = 180.f;
			List.Add(N2);

			FShiftNightSpec N3;
			N3.Title = TEXT("NIGHT 3");
			N3.Intro = TEXT("Final shift. All three rules are active tonight, and the old stain on this store is bleeding through.\nTrust the Shift Note [Tab], watch every visitor, and survive until 06:00 AM.");
			N3.Customers = { Customer(TEXT("Cleaner"), true, TEXT("Where do you keep the cleaner?")),
				Monster(TEXT("Sauce"), TEXT("Milk"), TEXT("Soy sauce. Where."), TEXT("...White and cold from the dairy carton... not dark...")), Customer(TEXT("Biscuits"), false),
				Customer(TEXT("Water"), true, TEXT("Sorry, where's the water?")),
				Monster(TEXT("Noodles"), TEXT("Bread"), TEXT("Something warm... please."), TEXT("...Soft sliced loaf from the morning bakery... no broth...")) };
			N3.OpeningTasks = { 1, 7, 9 };
			N3.Spills = 3; N3.TaskTimeLimit = 120.f;
			List.Add(N3);
		}
		return List;
	}
}
