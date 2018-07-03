//  This file is part of Project Name
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE

#include "web/web.h"
#include "../MapElitesScopeGP_World.h"

namespace UI = emp::web;
MapElitesGPConfig config;

UI::Document world_display("emp_base");
UI::Document program_info("program_info");
UI::Document program_exec("program_exec");
UI::Document settings("settings");

UI::Selector problem;
UI::TextArea genome_size;
UI::TextArea n_test_cases;
UI::TextArea inst_mut_rate;
UI::TextArea arg_mut_rate;

UI::Canvas canvas;
// UI::Div program_info;
const double world_width = 800;
const double world_height = 800;

MapElitesScopeGPWorld world;

void DrawWorldCanvas() {
  // UI::Canvas canvas = doc.Canvas("world_canvas");
  canvas.Clear("gray");

  const size_t world_x = world.GetWidth();
  const size_t world_y = world.GetHeight();
  const double canvas_x = (double) canvas.GetWidth();
  const double canvas_y = (double) canvas.GetHeight();
  std::cout << "x: " << world_x << " y: " << world_y << std::endl;
  const double org_x = canvas_x / (double) world_x;
  const double org_y = canvas_y / (double) world_y;
  const double org_r = emp::Min(org_x, org_y) / 2.0;

  for (size_t y = 0; y < world_y; y++) {
    for (size_t x = 0; x < world_x; x++) {
      const size_t org_id = y * world_x + x;
      const size_t cur_x = org_x * (0.5 + (double) x);
      const size_t cur_y = org_y * (0.5 + (double) y);
      const double fitness = world.CalcFitnessID(org_id);
      if (fitness == 0.0) {
        canvas.Rect(x*org_x, y*org_y, org_x, org_y, "#444444", "black");
      } else if (fitness < 10) {
        canvas.Rect(x*org_x, y*org_y, org_x, org_y, "pink", "black");
      } else if (fitness < 100) {
        canvas.Rect(x*org_x, y*org_y, org_x, org_y, "#EEEE33", "black");  // Pale Yellow
      } else if (fitness < 1000) {
        canvas.Rect(x*org_x, y*org_y, org_x, org_y, "#88FF88", "black");  // Pale green
      } else if (fitness < 9000) {
        canvas.Rect(x*org_x, y*org_y, org_x, org_y, "#00CC00", "black");  // Mid green
      } else {
        canvas.Rect(x*org_x, y*org_y, org_x, org_y, "green", "black");    // Full green
      }
    }
  }

  // Add a plus sign in the middle.
  // const double mid_x = org_x * world_x / 2.0;
  // const double mid_y = org_y * world_y / 2.0;
  // const double plus_bar = org_r * world_x;
  // canvas.Line(mid_x, mid_y-plus_bar, mid_x, mid_y+plus_bar, "#8888FF");
  // canvas.Line(mid_x-plus_bar, mid_y, mid_x+plus_bar, mid_y, "#8888FF");

  // doc.Text("ud_text").Redraw();
}

void ExecuteProgram(emp::AvidaGP & org) {
  program_exec.Clear();
  program_exec.SetAttr("class", "card-body visible");
  program_exec << "<h3 class='card-title'>Program execution</h3>";
  std::stringstream ss;
  std::stringstream temp_ss;
  ss << "</h5> <p class='card-text'>";
  org.PrintState(temp_ss);
  ss << emp::text2html(temp_ss.str());
  ss << "</p>";
  program_exec << ss.str();
  program_exec << UI::Button( [&org](){ org.SingleProcess(); ExecuteProgram(org); }, "Step", "step_button").SetAttr("class", "btn btn-primary");
}

void CanvasClick(int x, int y) {
  program_info.Clear();
  program_info.SetAttr("class", "card-body");
  
  // std::cout << "x: " << in_x << " y: " << in_y  <<std::endl;
  // double x = canvas.GetAdjustedX(in_x);
  // double y = canvas.GetAdjustedY(in_y);

  // UI::Canvas canvas = doc.Canvas("world_canvas");
  const double canvas_x = (double) canvas.GetWidth();
  const double canvas_y = (double) canvas.GetHeight();
  double px = ((double) x) / canvas_x;
  double py = ((double) y) / canvas_y;

  const size_t world_x = world.GetWidth();
  const size_t world_y = world.GetHeight();
  size_t pos_x = (size_t) (world_x * px);
  size_t pos_y = (size_t) (world_y * py);
  std::cout << "x: " << x << " y: " << y << "world_x: " << world_x << " world_y: " << world_y << " canvas_x: " << canvas_x <<" canvas_y: " << canvas_y  << " px: " << px <<  " py: " << py <<" pos_x: " << pos_x << " pos_y: " << pos_y <<std::endl;
  size_t org_id = pos_y * world_x + pos_x;
  std::stringstream ss;
  std::stringstream temp_ss;
  if (world.CalcFitnessID(org_id) > 0.0) {
    double entropy = world.inst_ent_fun(world.GetOrg(org_id));
    int scope_count = world.scope_count_fun(world.GetOrg(org_id));
    ss << "<h3 class='card-title'>Program information</h3>";
    ss << "<h5 class='card-subtitle mb-2 '>Fitness: " << world.CalcFitnessID(org_id)/1000.0 << " Scopes: " << scope_count << " Entropy: " << entropy << "</h5> <p class='card-text'>";
    world[org_id].PrintGenome(temp_ss);
    ss << emp::text2html(temp_ss.str());

    // program_info << UI::Text() << ss.str();
    program_info << ss.str();
    program_info << UI::Button( [org_id](){ 
      world.GetOrg(org_id).ResetHardware();
      world.GetOrg(org_id).SetInput(0, 1);
      ExecuteProgram(world.GetOrg(org_id)); 
    }, "Execute", "exec_button").SetAttr("class", "btn btn-primary");
    program_info << "</p>";

  } else {
    program_info << "<p class='card-text'>Click on a grid cell to see the program inside<p>";
    std::cout << "No org here" << std::endl;
  }

  // emp::Alert("Click at (", pos_x, ",", pos_y, ") = ", org_id);
}


int main()
{
  world_display << "<h2>Evolving ScopeGP Programs with MAP-Elites</h2>";
  program_info.SetAttr("class", "card-body");
  program_info << "<p class='card-text'>Click on a grid cell to see the program inside<p>";
  world.Setup(config);

  program_exec.SetAttr("class", "card-body invisible");

  problem.SetOption("Square", [](){config.TESTCASES_FPATH("../configs/testcases/examples-squares.csv"); world.Setup(config);DrawWorldCanvas();});
  problem.SetOption("Count odds", [](){config.TESTCASES_FPATH("../configs/testcases/count-odds.csv"); world.Setup(config);DrawWorldCanvas();});

  n_test_cases.SetCallback([](const std::string & curr){config.NUM_TEST_CASES(emp::from_string<int>(curr)); world.Setup(config);DrawWorldCanvas();});
  // genome_size.SetCallback([](const std::string & curr){config.GENOME_SIZE(emp::from_string<int>(curr)); world.Setup(config);DrawWorldCanvas();});
  inst_mut_rate.SetCallback([](const std::string & curr){config.INST_SUB__PER_INST(emp::from_string<double>(curr)); world.Setup(config);DrawWorldCanvas();});
  arg_mut_rate.SetCallback([](const std::string & curr){config.ARG_SUB__PER_ARG(emp::from_string<double>(curr)); world.Setup(config);DrawWorldCanvas();});

  settings << "Problem: " << problem << "<br>";
  settings << "Number of test cases: " << n_test_cases << "<br>";
  // settings << "Genome length: " << genome_size << "<br>";
  settings << "Instruction mutation rate: " << inst_mut_rate << "<br>";
  settings << "Argument mutation rate: " << arg_mut_rate << "<br>";

  // Add some Buttons
  world_display << UI::Button( [](){ emp::RandomSelect(world, 1); DrawWorldCanvas(); }, "Reproduce", "birth_button").SetAttr("class", "btn btn-primary");
  world_display << UI::Button( [](){ emp::RandomSelect(world, 100); DrawWorldCanvas(); }, "Reproduce 100", "birth_100_button").SetAttr("class", "btn btn-primary ml-1");
  world_display << UI::Button( [](){ emp::RandomSelect(world, 1000); DrawWorldCanvas(); }, "Reproduce 1000", "birth_1000_button").SetAttr("class", "btn btn-primary ml-1");
  world_display << UI::Button( [](){ emp::RandomSelect(world, 10000); DrawWorldCanvas(); }, "Reproduce 10000", "birth_10000_button").SetAttr("class", "btn btn-primary ml-1");
  world_display << "<br><br>";
  
  canvas = world_display.AddCanvas(world_width, world_height, "world_canvas");
  // program_info = program_display.AddDiv("program_info");
  // program_info.SetCSS("right", 0);
  canvas.On("click", CanvasClick);
  DrawWorldCanvas();
}
