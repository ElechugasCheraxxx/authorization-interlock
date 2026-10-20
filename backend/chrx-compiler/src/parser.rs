use crate::ast::*;
use crate::Token;
use logos::{Lexer, Logos};

pub struct Parser<'a> {
    lexer: Lexer<'a, Token>,
    current_token: Option<Token>,
}

#[derive(Debug, thiserror::Error)]
pub enum ParserError {
    #[error("Unexpected token: {0:?}")]
    UnexpectedToken(Option<Token>),
    #[error("Expected {0}, found {1:?}")]
    ExpectedToken(String, Option<Token>),
    #[error("Lexer error")]
    LexerError,
}

impl<'a> Parser<'a> {
    pub fn new(input: &'a str) -> Self {
        let mut lexer = Token::lexer(input);
        let current_token = lexer.next().and_then(|r| r.ok());
        Self { lexer, current_token }
    }

    fn advance(&mut self) {
        self.current_token = self.lexer.next().and_then(|r| r.ok());
    }

    fn expect(&mut self, expected: Token) -> Result<(), ParserError> {
        if self.current_token == Some(expected.clone()) {
            self.advance();
            Ok(())
        } else {
            Err(ParserError::ExpectedToken(format!("{:?}", expected), self.current_token.clone()))
        }
    }

    pub fn parse_mission(&mut self) -> Result<Mission, ParserError> {
        self.expect(Token::Mission)?;

        let name = if let Some(Token::String(n)) = self.current_token.take() {
            self.advance();
            n
        } else {
            return Err(ParserError::ExpectedToken("Mission Name (String)".to_string(), self.current_token.clone()));
        };

        self.expect(Token::BraceOpen)?;

        let mut directives = Vec::new();
        let mut verify_block = Vec::new();
        let mut resources = Vec::new();
        let mut swarms = Vec::new();
        let mut flows = Vec::new();
        let mut strict_limits = Vec::new();
        let mut choreographies = Vec::new();
        let mut swarm_syncs = Vec::new();

        while self.current_token != Some(Token::BraceClose) && self.current_token.is_some() {
            match self.current_token {
                Some(Token::CompilerDirective) | Some(Token::MemoryPoolDirective) | Some(Token::AlignmentDirective) | Some(Token::TemporalDriftDirective) => {
                    directives.push(self.parse_directive()?);
                }
                Some(Token::Verify) => {
                    verify_block.extend(self.parse_verify()?);
                }
                Some(Token::Resource) | Some(Token::Buffer) => {
                    resources.push(self.parse_resource()?);
                }
                Some(Token::Swarm) => {
                    swarms.push(self.parse_swarm()?);
                }
                Some(Token::Flow) => {
                    flows.push(self.parse_flow()?);
                }
                Some(Token::StrictLimit) => {
                    strict_limits.push(self.parse_strict_limit()?);
                }
                Some(Token::Choreography) => {
                    choreographies.push(self.parse_choreography()?);
                }
                Some(Token::SwarmSync) => {
                    swarm_syncs.push(self.parse_swarm_sync()?);
                }
                _ => {
                    return Err(ParserError::UnexpectedToken(self.current_token.clone()));
                }
            }
        }

        self.expect(Token::BraceClose)?;
        Ok(Mission { name, directives, verify_block, resources, swarms, flows, strict_limits, choreographies, swarm_syncs })
    }

    fn parse_directive(&mut self) -> Result<Directive, ParserError> {
        match self.current_token.take() {
            Some(Token::CompilerDirective) => {
                self.advance();
                self.expect(Token::BraceOpen)?;
                let mut target = "M4_PRO".to_string();
                let mut optimization = "O3".to_string();
                let mut integrity = None;
                let mut armor = None;

                while self.current_token != Some(Token::BraceClose) {
                    let key = self.expect_ident()?;
                    self.expect(Token::Colon)?;
                    let value = self.expect_value()?;
                    self.expect(Token::Semicolon)?;

                    match key.as_str() {
                        "target" => target = value,
                        "optimization" => optimization = value,
                        "integrity" => integrity = Some(value),
                        "armor" => armor = Some(value),
                        _ => {}
                    }
                }
                self.expect(Token::BraceClose)?;
                Ok(Directive::Compiler { target, optimization, integrity, armor })
            }
            Some(Token::MemoryPoolDirective) => {
                self.advance();
                let name = self.expect_ident()?;
                self.expect(Token::BracketOpen)?;
                let size = self.expect_literal()?;
                self.expect(Token::BracketClose)?;
                self.expect(Token::Semicolon)?;
                Ok(Directive::MemoryPool { name, size })
            }
            Some(Token::AlignmentDirective) => {
                self.advance();
                let bytes_str = self.expect_literal()?;
                let bytes = bytes_str.replace("B", "").parse().unwrap_or(64);
                self.expect(Token::Semicolon)?;
                Ok(Directive::Alignment { bytes })
            }
            Some(Token::TemporalDriftDirective) => {
                self.advance();
                let value = self.expect_literal()?;
                self.expect(Token::Semicolon)?;
                Ok(Directive::TemporalDrift { value })
            }
            _ => Err(ParserError::UnexpectedToken(self.current_token.clone())),
        }
    }

    fn parse_verify(&mut self) -> Result<Vec<Verification>, ParserError> {
        self.advance(); // consume 'verify'
        self.expect(Token::BraceOpen)?;
        let mut verifications = Vec::new();
        while self.current_token != Some(Token::BraceClose) {
            match self.current_token.clone() {
                Some(Token::Invariant) => {
                    self.advance();
                    verifications.push(Verification::Invariant(self.expect_ident()?));
                    self.expect(Token::Semicolon)?;
                }
                Some(Token::Precondition) => {
                    self.advance();
                    verifications.push(Verification::Precondition(self.expect_ident()?));
                    self.expect(Token::Semicolon)?;
                }
                Some(Token::Postcondition) => {
                    self.advance();
                    verifications.push(Verification::Postcondition(self.expect_ident()?));
                    self.expect(Token::Semicolon)?;
                }
                _ => break,
            }
        }
        self.expect(Token::BraceClose)?;
        Ok(verifications)
    }

    fn parse_resource(&mut self) -> Result<Resource, ParserError> {
        let is_buffer = self.current_token == Some(Token::Buffer);
        self.advance(); // consume 'resource' or 'buffer'

        let mut name = String::new();
        let mut size = "0B".to_string();
        let mut alignment = 64;
        let mut r_type = if is_buffer { "BUFFER".to_string() } else { "VOLATILE_POOL".to_string() };

        if is_buffer {
            self.expect(Token::BracketOpen)?;
            size = self.expect_literal()?;
            self.expect(Token::BracketClose)?;
            name = self.expect_ident()?;
            if self.current_token == Some(Token::Colon) {
                self.advance();
                r_type = format!("BUFFER_{}", self.expect_ident()?.to_uppercase());
            }
            // Buffers in the example don't have braces
            return Ok(Resource { name, r_type, size, alignment });
        } else {
            name = self.expect_ident()?;
            self.expect(Token::BraceOpen)?;
            while self.current_token != Some(Token::BraceClose) {
                let key = self.expect_ident()?;
                self.expect(Token::Colon)?;
                match key.as_str() {
                    "type" => r_type = self.expect_value()?,
                    "size" => size = self.expect_literal()?,
                    "align" | "alignment" => {
                        let val = self.expect_literal()?;
                        alignment = val.replace("B", "").parse().unwrap_or(64);
                    }
                    _ => { self.advance(); }
                }
                self.expect(Token::Semicolon)?;
            }
            self.expect(Token::BraceClose)?;
        }
        Ok(Resource { name, r_type, size, alignment })
    }

    fn parse_strict_limit(&mut self) -> Result<StrictLimit, ParserError> {
        self.advance(); // consume 'strict_limit'
        let key = self.expect_ident()?;
        let condition = match self.current_token.clone() {
            Some(Token::Less) => { self.advance(); "<".to_string() }
            Some(Token::Greater) => { self.advance(); ">".to_string() }
            Some(Token::LessEqual) => { self.advance(); "<=".to_string() }
            Some(Token::GreaterEqual) => { self.advance(); ">=".to_string() }
            _ => return Err(ParserError::ExpectedToken("Comparison Operator".to_string(), self.current_token.clone())),
        };
        let value = self.expect_literal()?;
        Ok(StrictLimit { key, condition, value })
    }

    fn parse_choreography(&mut self) -> Result<Choreography, ParserError> {
        self.advance(); // consume 'choreography'
        let name = self.expect_ident()?;
        self.expect(Token::BraceOpen)?;
        let mut phases = Vec::new();
        while self.current_token == Some(Token::Phase) {
            phases.push(self.parse_phase()?);
        }
        self.expect(Token::BraceClose)?;
        Ok(Choreography { name, phases })
    }

    fn parse_phase(&mut self) -> Result<Phase, ParserError> {
        self.advance(); // consume 'phase'
        let name = self.expect_ident()?;
        self.expect(Token::Colon)?;
        let mut commands = Vec::new();
        // Commands continue until next 'phase' or '}'
        while self.current_token != Some(Token::Phase) && self.current_token != Some(Token::BraceClose) {
            commands.push(self.parse_command()?);
        }
        Ok(Phase { name, commands })
    }

    fn parse_command(&mut self) -> Result<ChrxCommand, ParserError> {
        match self.current_token.clone() {
            Some(Token::Bind) => {
                self.advance();
                let val = self.expect_ident()?;
                if self.current_token == Some(Token::ParenOpen) {
                    self.advance();
                    // Basic arg parsing for now
                    let arg = self.expect_ident()?;
                    self.expect(Token::ParenClose)?;
                    Ok(ChrxCommand::Bind(format!("{}({})", val, arg)))
                } else {
                    Ok(ChrxCommand::Bind(val))
                }
            }
            Some(Token::Await) => {
                self.advance();
                Ok(ChrxCommand::Await(self.expect_ident()?))
            }
            Some(Token::Lock) => {
                self.advance();
                self.expect(Token::ParenOpen)?;
                let val = self.expect_ident()?;
                self.expect(Token::ParenClose)?;
                Ok(ChrxCommand::Lock(val))
            }
            Some(Token::Assert) => {
                self.advance();
                let mut condition = self.expect_ident()?;
                if self.current_token == Some(Token::Equals) {
                    self.advance();
                    condition.push_str(" == ");
                    condition.push_str(&self.expect_ident()?);
                }
                Ok(ChrxCommand::Assert(condition))
            }
            Some(Token::Purge) => {
                self.advance();
                Ok(ChrxCommand::Purge(self.expect_ident()?))
            }
            Some(Token::Halt) => {
                self.advance();
                Ok(ChrxCommand::Halt(self.expect_ident()?))
            }
            Some(Token::Commit) => {
                self.advance();
                self.expect(Token::ParenOpen)?;
                let val = self.expect_ident()?;
                self.expect(Token::ParenClose)?;
                Ok(ChrxCommand::Commit(val))
            }
            Some(Token::Resume) => {
                self.advance();
                Ok(ChrxCommand::Resume(self.expect_ident()?))
            }
            Some(Token::Ident(id)) => {
                self.advance();
                if self.current_token == Some(Token::Arrow) {
                    self.advance();
                    let target = self.expect_ident()?;
                    Ok(ChrxCommand::Call(format!("{} -> {}", id, target)))
                } else if self.current_token == Some(Token::ParenOpen) {
                    self.advance();
                    self.expect(Token::ParenClose)?;
                    Ok(ChrxCommand::Call(id))
                } else {
                    Ok(ChrxCommand::Call(id))
                }
            }
            _ => Err(ParserError::UnexpectedToken(self.current_token.clone())),
        }
    }

    fn parse_swarm_sync(&mut self) -> Result<SwarmSync, ParserError> {
        self.advance(); // consume 'swarm_sync'
        self.expect(Token::BraceOpen)?;
        let mut events = Vec::new();
        while self.current_token == Some(Token::OnEvent) {
            events.push(self.parse_sync_event()?);
        }
        self.expect(Token::BraceClose)?;
        Ok(SwarmSync { events })
    }

    fn parse_sync_event(&mut self) -> Result<SyncEvent, ParserError> {
        self.advance(); // consume 'on_event'
        self.expect(Token::ParenOpen)?;
        let event_name = self.expect_ident()?;
        self.expect(Token::ParenClose)?;
        self.expect(Token::Colon)?;
        let mut commands = Vec::new();
        // Commands until next 'on_event' or '}'
        while self.current_token != Some(Token::OnEvent) && self.current_token != Some(Token::BraceClose) {
            commands.push(self.parse_command()?);
        }
        Ok(SyncEvent { event_name, commands })
    }

    fn parse_swarm(&mut self) -> Result<Swarm, ParserError> {
        self.advance(); // consume 'swarm'
        let name = self.expect_ident()?;
        self.expect(Token::BraceOpen)?;
        let mut agents = Vec::new();
        while self.current_token == Some(Token::Agent) {
            agents.push(self.parse_agent()?);
        }
        self.expect(Token::BraceClose)?;
        Ok(Swarm { name, agents })
    }

    fn parse_agent(&mut self) -> Result<Agent, ParserError> {
        self.advance(); // consume 'agent'
        let name = self.expect_ident()?;
        self.expect(Token::BraceOpen)?;
        let mut role = "EXECUTION".to_string();
        let mut artifact = None;
        let mut integrity = None;
        let mut target = None;
        let mut trigger = None;
        let mut on_success = None;
        let mut on_fail = None;
        let mut tasks = Vec::new();

        while self.current_token != Some(Token::BraceClose) {
            match self.current_token.clone() {
                Some(Token::OnSuccess) => {
                    self.advance();
                    self.expect(Token::Arrow)?;
                    on_success = Some(self.expect_ident()?);
                    self.expect(Token::Semicolon)?;
                }
                Some(Token::OnFail) => {
                    self.advance();
                    self.expect(Token::Arrow)?;
                    on_fail = Some(self.expect_ident()?);
                    self.expect(Token::Semicolon)?;
                }
                _ => {
                    let key = self.expect_ident()?;
                    self.expect(Token::Colon)?;
                    let value = self.expect_value()?;
                    self.expect(Token::Semicolon)?;
                    match key.as_str() {
                        "role" => role = value,
                        "artifact" => artifact = Some(value),
                        "integrity" => integrity = Some(value),
                        "target" => target = Some(value),
                        "trigger" => trigger = Some(value),
                        "task" => tasks.push(value),
                        _ => {}
                    }
                }
            }
        }

        self.expect(Token::BraceClose)?;
        Ok(Agent { name, role, artifact, integrity, target, trigger, on_success, on_fail, tasks })
    }

    fn parse_flow(&mut self) -> Result<Flow, ParserError> {
        self.advance(); // consume 'flow'
        let name = self.expect_ident()?;
        self.expect(Token::BraceOpen)?;
        let mut steps = Vec::new();
        let mut links = Vec::new();

        while self.current_token != Some(Token::BraceClose) {
            match self.current_token.clone() {
                Some(Token::Step) => {
                    self.advance();
                    let id_str = self.expect_literal()?;
                    let id = id_str.parse().unwrap_or(0);
                    self.expect(Token::Colon)?;
                    let action = if let Some(Token::Ident(a)) = self.current_token.take() {
                        self.advance();
                        a
                    } else if let Some(Token::String(a)) = self.current_token.take() {
                        self.advance();
                        a
                    } else {
                        return Err(ParserError::ExpectedToken("Action".to_string(), self.current_token.clone()));
                    };
                    self.expect(Token::Semicolon)?;
                    steps.push(Step { id, action });
                }
                Some(Token::Link) => {
                    self.advance();
                    let from_str = self.expect_literal()?;
                    let from = from_str.parse().unwrap_or(0);
                    self.expect(Token::Arrow)?;
                    let to_str = self.expect_literal()?;
                    let to = to_str.parse().unwrap_or(0);
                    let mut condition = "TRUE".to_string();
                    if self.current_token == Some(Token::When) || self.current_token == Some(Token::If) {
                        self.advance();
                        condition = self.expect_value()?;
                    }
                    self.expect(Token::Semicolon)?;
                    links.push(Link { from, to, condition });
                }
                _ => self.advance(),
            }
        }

        self.expect(Token::BraceClose)?;
        Ok(Flow { name, steps, links })
    }

    fn expect_ident(&mut self) -> Result<String, ParserError> {
        if let Some(Token::Ident(id)) = self.current_token.take() {
            self.advance();
            Ok(id)
        } else {
            Err(ParserError::ExpectedToken("Identifier".to_string(), self.current_token.clone()))
        }
    }

    fn expect_literal(&mut self) -> Result<String, ParserError> {
        if let Some(Token::Literal(lit)) = self.current_token.take() {
            self.advance();
            Ok(lit)
        } else {
            Err(ParserError::ExpectedToken("Literal".to_string(), self.current_token.clone()))
        }
    }

    fn expect_value(&mut self) -> Result<String, ParserError> {
        match self.current_token.take() {
            Some(Token::Ident(v)) | Some(Token::Literal(v)) | Some(Token::String(v)) => {
                self.advance();
                Ok(v)
            }
            _ => Err(ParserError::ExpectedToken("Value (Ident, Literal, or String)".to_string(), self.current_token.clone())),
        }
    }
}
