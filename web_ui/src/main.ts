import './style.css'
import { SocialState } from './social/state.ts';
import { AppState } from './app.ts';
import { MsgState } from './message/state.ts';
import { MARKET } from './macros.ts';
import { MarketState } from './market/state.ts';


let app = new AppState();

let social_state = new SocialState(app);
app.pages.push(social_state);

let message_state = new MsgState(app);
app.pages.push(message_state);

let market_state = new MarketState(app);
app.pages.push(market_state);


app.header.load_pages(app);
app.render_page(MARKET);
