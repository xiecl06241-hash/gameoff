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
			auto Monster = [](const TCHAR* Sku, const TCHAR* Lost, const TCHAR* Line) {
				FShiftCustomerSpec C; C.WantedSku = Sku; C.bAsksHelp = true; C.bMonster = true; C.MonsterSku = Lost; C.Line = Line; return C;
			};

			FShiftNightSpec N1;
			N1.Title = TEXT("NIGHT 1");
			N1.Intro = TEXT("First night shift. Finish your chores, serve whoever walks in,\nclean up, and hand the store over at 06:00.");
			N1.Customers = { Customer(TEXT("Water"), false), Customer(TEXT("Noodles"), true, TEXT("Excuse me... where are the noodle cups?")), Customer(TEXT("Chips"), false) };
			N1.OpeningTasks = { 3, 5 };
			N1.Spills = 2; N1.TaskTimeLimit = 70.f; N1.bEerieEvents = true;
			List.Add(N1);

			FShiftNightSpec N2;
			N2.Title = TEXT("NIGHT 2");
			N2.Intro = TEXT("Note from the day shift: \"Some customers are not customers.\n If the lights go out, give them what they want. Don't let them touch you.\"");
			N2.Customers = { Customer(TEXT("Milk"), false), Customer(TEXT("Bread"), true, TEXT("Do you have any bread left?")),
				Monster(TEXT("Soda"), TEXT("Can"), TEXT("I... need something to drink.")), Customer(TEXT("Meal"), false) };
			N2.OpeningTasks = { 0, 4 };
			N2.Spills = 3; N2.TaskTimeLimit = 60.f;
			List.Add(N2);

			FShiftNightSpec N3;
			N3.Title = TEXT("NIGHT 3");
			N3.Intro = TEXT("Last night of the week. The store keeps changing when they come.\nRemember where everything was.");
			N3.Customers = { Customer(TEXT("Cleaner"), true, TEXT("Where do you keep the cleaner?")),
				Monster(TEXT("Sauce"), TEXT("Milk"), TEXT("Soy sauce. Where.")), Customer(TEXT("Biscuits"), false),
				Customer(TEXT("Water"), true, TEXT("Sorry, where's the water?")),
				Monster(TEXT("Noodles"), TEXT("Bread"), TEXT("Something warm... please.")) };
			N3.OpeningTasks = { 1, 7, 9 };
			N3.Spills = 3; N3.TaskTimeLimit = 55.f;
			List.Add(N3);
		}
		return List;
	}
}
